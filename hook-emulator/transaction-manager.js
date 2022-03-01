const { exec } = require("child_process");
const codec = require('ripple-address-codec');
const rippleCodec = require('ripple-binary-codec');
const { STATE_ERROR_CODES } = require('./state-manager');
const { XflHelpers } = require('evernode-js-client');

const TX_WAIT_TIMEOUT = 5000;
const TX_MAX_LEDGER_OFFSET = 10;
const TX_NFT_MINT = 'NFTokenMint'

const ErrorCodes = {
    TIMEOUT: 'TIMEOUT',
    TX_ERROR: 'TX_ERROR'
}

const MESSAGE_TYPES = {
    TRACE: 0,
    EMIT: 1,
    TRUSTLINE: 2,
    STATE_GET: 3,
    STATE_SET: 4,
    ACCID: 5,
    SEQUENCE: 6,
    MINTED_TOKENS: 7
};

const RETURN_CODES = {
    SUCCESS: 0,
    INTERNAL_ERROR: -1,
    NOT_FOUND: -2,
    OVERFLOW: -3,
    UNDERFLOW: -4
}

// --------------- Message data formats ----------------

// ''''' JS --> C_WRAPPER (Write to STDIN from JS) '''''
// Transaction origin - <hookid(20)><hash(32)><account(20)><1 for xrp and 0 for iou(1)><[XRP: amount in buf(8)XFL][IOU: <issuer(20)><currency(3)><amount in buf(8)XFL>]><destination(20)><memo count(1)><[<TypeLen(1)><MemoType(20)><FormatLen(1)><MemoFormat(20)><DataLen(1)><MemoData(128)>]><ledger_hash(32)><ledger_index(8)>
// Message response format - <RETURN CODE(1)><DATA>
// Emit response - <return code(1)><tx hash(32)>
// Trustline response - <return code(1)><balance(8)XFL><limit(8)XFL>
// State get response - <return code(1)><value(128)>
// State set response - <return code(1)>
// '''''''''''''''''''''''''''''''''''''''''''''''''''''

// ' C_WRAPPER --> JS (Write to STDOUT from C_WRAPPER) '
// Message protocol - <data len(4)><data>
// Message request format - <TYPE(1)><DATA>
// Trace request - <type:TRACE(1)><trace message>
// Transaction emit request - <type:EMIT(1)><prepared transaction buf>
// Trustline request - <type:TRUSTLINE(1)><address(20)><issuer(20)><currency(3)>
// State set request - <type:STATE_GET(1)><key(32)><value(128)>
// State get request - <type:STATE_SET(1)><key(32)>
// '''''''''''''''''''''''''''''''''''''''''''''''''''''
// -----------------------------------------------------

/**
 * Transaction manager handles the transactions.
 * Executes the hook as a child process and commmunicate with child process using STDIN and STDOUT.
 */
class TransactionManager {
    #hookWrapperPath = null;
    #stateManager = null;
    #accountManager = null;
    #hookAccount = null;
    #hookProcess = null;
    #draftEmits = null;
    #pendingTransactions = null;
    #queueProcessorActive = null;

    constructor(hookAccount, hookWrapperPath, stateManager, accountManager) {
        this.#hookAccount = hookAccount;
        this.#hookWrapperPath = hookWrapperPath;
        this.#stateManager = stateManager;
        this.#accountManager = accountManager;
        this.#pendingTransactions = [];
        this.#queueProcessorActive = false;
        this.#initDrafts();
    }

    async init() {
        await this.#stateManager.init();
        this.#accountManager.init();
    }

    #initDrafts() {
        this.#draftEmits = [];
    }

    #encodeTransaction(transaction) {
        // Check whether transaction is a xrp transaction  or not.
        const isXrp = (typeof transaction.Amount === 'string');

        // Pre allocate a buffer to propulate transaction info. allocUnsafe since it's faster and the whole buffer will be allocated in the later part.
        let txBuf = Buffer.allocUnsafe(74 + (isXrp ? 8 : 31))

        let offset = 0;
        // Populate the transaction hash (32-bytes).
        Buffer.from(transaction.hash, 'hex').copy(txBuf, offset);
        offset += 32;

        // Get origin account id from r-address and populate (20-bytes).
        codec.decodeAccountID(transaction.Account).copy(txBuf, offset);
        offset += 20;

        // Amount buf -> if transaction is a xrp transaction <amount|xfl(8-bytes)> otherwise <issuer(20-bytes)><currency(30-bytes)><value|xfl(8-bytes)>
        if (isXrp) {
            txBuf.writeUInt8(1, offset++);
            const amount = parseFloat(transaction.Amount) / 1000000;
            // Convert amount to xfl and populate.
            txBuf.writeBigInt64BE(XflHelpers.getXfl(amount.toString()), offset);
            offset += 8;
        }
        else {
            txBuf.writeUInt8(0, offset++);
            // Get issuer account id from r-address and populate.
            codec.decodeAccountID(transaction.Amount.issuer).copy(txBuf, offset);
            offset += 20;
            Buffer.from(transaction.Amount.currency).copy(txBuf, offset);
            offset += 3;
            // Convert amount value to xfl and populate.
            txBuf.writeBigInt64BE(XflHelpers.getXfl(transaction.Amount.value), offset);
            offset += 8;
        }

        // Get destination account id from r-address and populate (20-bytes).
        codec.decodeAccountID(transaction.Destination).copy(txBuf, offset);
        offset += 20;

        const memos = transaction.Memos ? transaction.Memos : [];

        // Populate memo count (1-byte).
        txBuf.writeUInt8(memos.length, offset++);

        for (const memo of memos) {
            const typeBuf = Buffer.from(memo.type ? memo.type : []);
            const formatBuf = Buffer.from(memo.format ? memo.format : []);
            const dataBuf = memo.data ? Buffer.from(memo.data, memo.format === 'hex' ? 'hex' : 'utf8') : Buffer.from([]);

            // Pre allcate buffer to populate memo info.
            let memoBuf = Buffer.allocUnsafe(3 + typeBuf.length + formatBuf.length + dataBuf.length);

            // Populate type length and type buffer ((1 + typeLen)-bytes).
            let memoBufOffset = 0;
            memoBuf.writeUInt8(typeBuf.length, memoBufOffset++);
            typeBuf.copy(memoBuf, memoBufOffset);
            memoBufOffset += typeBuf.length;

            // Populate format length and format buffer ((1 + formatLen)-bytes).
            memoBuf.writeUInt8(formatBuf.length, memoBufOffset++);
            formatBuf.copy(memoBuf, memoBufOffset);
            memoBufOffset += formatBuf.length;

            // Populate data length and data buffer ((1 + dataLen)-bytes).
            memoBuf.writeUInt8(dataBuf.length, memoBufOffset++);
            dataBuf.copy(memoBuf, memoBufOffset);
            memoBufOffset += dataBuf.length;

            // Append memo buf to the transaction buf.
            txBuf = Buffer.concat([txBuf, memoBuf]);
        }

        // Pre allocate the return buf.
        let returnBuf = Buffer.allocUnsafe(txBuf.length + 40);

        offset = 0;

        // Populate prepared transaction buf to the return buf ((txBufLen)-bytes).
        txBuf.copy(returnBuf, offset);
        offset += txBuf.length;

        // Populate ledger hash to the return buf (32-bytes).
        Buffer.from(transaction.LedgerHash, 'hex').copy(returnBuf, offset);
        offset += 32;

        // Populate ledger index to the return buf (8-bytes).
        returnBuf.writeBigUInt64BE(BigInt(transaction.LedgerIndex), offset);

        return returnBuf;
    }

    #encodeTrustLines(trustLines) {
        // Pre allocate buffer to populate trustline data.
        let buf = Buffer.allocUnsafe(16)

        let offset = 0;
        // Convert balance to xfl and populate (8-bytes).
        buf.writeBigInt64BE(XflHelpers.getXfl(trustLines[0].balance), offset);
        offset += 8;
        // Convert limit to xfl and populate (8-bytes).
        buf.writeBigInt64BE(XflHelpers.getXfl(trustLines[0].limit), offset);
        offset += 8;

        return buf;
    }

    #encodeUint32(number) {
        // Pre allocate buffer to populate number data.
        let buf = Buffer.allocUnsafe(4)
        buf.writeUInt32BE(number, 0);
        return buf;
    }

    #encodeReturnCode(retCode, dataBuf = null) {
        let resBuf = Buffer.alloc((dataBuf ? dataBuf.length : 0) + 1);
        resBuf.writeInt8(retCode);
        if (dataBuf)
            dataBuf.copy(resBuf, 1);
        return resBuf;
    }

    #decodeTrustlineRequest(requestBuf) {
        let request = {};

        let offset = 0;
        // Get the address and encode the r-address (20-bytes)
        request.address = codec.encodeAccountID(requestBuf.slice(offset, offset + 20));
        offset += 20;

        // Get the issuer and encode the r-address (20-bytes)
        request.issuer = codec.encodeAccountID(requestBuf.slice(offset, offset + 20));
        offset += 20;

        // Get the currency (3-bytes).
        request.currency = requestBuf.slice(offset, offset + 3).toString();

        return request;
    }

    #decodeStateGetRequest(requestBuf) {
        let request = {};

        // Get the key (32-bytes).
        request.key = requestBuf.slice(0, 32);

        return request;
    }

    #decodeStateSetRequest(requestBuf) {
        let request = {};

        let offset = 0;
        // Get the key (32-bytes).
        request.key = requestBuf.slice(offset, offset + 32);
        offset += 32;

        // Get the value (128-bytes).
        request.value = requestBuf.slice(offset, offset + 128);

        return request;
    }

    #sendToProc(data) {
        this.#hookProcess.stdin.write(data);
    }

    processTransaction(transaction) {
        const resolver = new Promise((resolve, reject) => {
            this.#pendingTransactions.push({
                resolve: resolve,
                reject: reject,
                transaction: transaction
            });
        });
        console.log("Queued a transaction.");

        // Start queue processor when item is added.
        this.#processQueue();

        return resolver;
    }

    #decodeTransactionBuf(transactionBuf) {
        let txJson = rippleCodec.decode(transactionBuf.toString('hex'));
        // First ledger sequence and signing pub key are populated by hook. So we remove those.
        delete txJson.FirstLedgerSequence;
        delete txJson.SigningPubKey;
        return txJson;
    }

    async #processQueue() {
        if (this.#queueProcessorActive)
            return;

        console.log("Transaction queue processor started...");
        this.#queueProcessorActive = true;

        let i = 1;
        while (this.#pendingTransactions && this.#pendingTransactions.length > 0) {
            console.log(`Processing transaction ${i}...`);
            const task = this.#pendingTransactions.shift();

            try {
                const ret = await this.#executeHook(task.transaction);
                await this.#persistTransaction().catch(console.error);
                task.resolve(ret);
            }
            catch (e) {
                await this.#rollbackTransaction(task.transaction).catch(console.error);
                task.reject(e);
            }

            // Close the STDIN and clear the hook process.
            this.#hookProcess.stdin.end();
            this.#hookProcess = null;
            this.#initDrafts();

            console.log(`Processed transaction  ${i}.`);
            i++;
        }

        this.#queueProcessorActive = false;
        console.log("Transaction queue processor ended.");
    }

    async #executeHook(transaction) {
        // Encode the transaction info to the buf.
        let txBuf = this.#encodeTransaction(transaction);

        // Get the hook account id from r-address and append to the buf.
        txBuf = Buffer.concat([codec.decodeAccountID(this.#hookAccount.address), txBuf]);

        // Execute the hook wrapper binary.
        this.#hookProcess = exec(this.#hookWrapperPath);

        // Set STDIN and STDOUT encoding to binary.
        this.#hookProcess.stdin.setEncoding('binary');
        this.#hookProcess.stdout.setEncoding('binary');
        // Set STDERR to the utf8.
        this.#hookProcess.stderr.setEncoding('utf8');

        // Writing transaction to stdin.
        this.#sendToProc(txBuf);

        return new Promise((resolve, reject) => {
            // This flag is used to mark the promise as completed, so events won't get listened further.
            let completed = false;

            // Set the transaction process timeout and reject the promise if reached.
            const failTimeout = setTimeout(() => {
                completed = true;
                reject(ErrorCodes.TIMEOUT);
            }, TX_WAIT_TIMEOUT);

            // This is fired when child process is exited.
            this.#hookProcess.on('close', async (code) => {
                if (!completed) {
                    completed = true;
                    clearTimeout(failTimeout);
                    // Resolve of success, otherwise reject with error.
                    if (code === 0)
                        resolve("SUCCESS");
                    else {
                        reject(ErrorCodes.TX_ERROR);
                    }
                }
            });

            // This is fired when child process is writing to the STDOUT.
            this.#hookProcess.stdout.on('data', async (data) => {
                if (!completed) {
                    const messageBuf = Buffer.from(data, "binary");

                    // Data chuncks will be prefixed (4-bytes) with the data length.
                    // Split data by lengths and handle seperately.
                    let offset = 0;
                    while (offset < messageBuf.length) {
                        const msgLen = messageBuf.readUInt32BE(offset);
                        offset += 4;

                        const msg = messageBuf.slice(offset, offset + msgLen);
                        offset += msgLen;

                        await this.#handleMessage(msg).catch(console.error);
                    }
                }
            });

            // This is fired when child process is writing to the STDOUT.
            this.#hookProcess.stderr.on('data', async (data) => {
                if (!completed) {
                    // Just print the error.
                    console.error(data);
                }
            });
        });
    }

    async #persistTransaction() {
        for (let transaction of this.#draftEmits) {
            try {
                // Update the sequence and last ledger sequence values.
                const sequence = await this.#hookAccount.getSequence();
                transaction.Sequence = sequence;
                transaction.LastLedgerSequence = this.#hookAccount.xrplApi.ledgerIndex + TX_MAX_LEDGER_OFFSET;
                await this.#hookAccount.xrplApi.submitAndVerify(transaction, { wallet: this.#hookAccount.wallet });
            }
            catch (e) {
                // If failed transaction is a NFT mint, Decrement the mint counter.
                if (transaction.TransactionType === TX_NFT_MINT)
                    this.#accountManager.decreaseMintedTokensSeq();
                console.error(e);
            }
        }
        await this.#stateManager.persist();
        this.#accountManager.persist();
    }

    async #rollbackTransaction(transaction) {
        // Send back the transaction amount to the sender.
        const isXrp = (typeof transaction.Amount === 'string');
        const amount = isXrp ? transaction.Amount : transaction.Amount.value;
        const currency = isXrp ? 'XRP' : transaction.Amount.currency;
        const issuer = isXrp ? null : transaction.Amount.issuer;
        await this.#hookAccount.makePayment(transaction.Account, amount, currency, issuer);

        // Rollback the state changes.
        this.#stateManager.rollback();
        this.#accountManager.rollback();
    }

    async #handleMessage(messageBuf) {
        // Get the message type from the message (1-byte).
        const type = messageBuf.readUInt8(0);
        // Get the message content from the message (rest of bytes).
        const content = messageBuf.slice(1);

        switch (type) {
            case (MESSAGE_TYPES.TRACE):
                // Only log the content.
                console.log(content.toString());
                break;
            case (MESSAGE_TYPES.EMIT):
                try {
                    const txJson = this.#decodeTransactionBuf(content);
                    // If transaction is a NFT mint, Increment the mint counter before adding to drafts.
                    if (txJson.TransactionType === TX_NFT_MINT)
                        this.#accountManager.increaseMintedTokensSeq();

                    // We cannot pre calculate the hash since we are updating the sequence and last ledger sequence at the transaction submission.
                    // So, send an empty buffer as the transaction hash.
                    this.#sendToProc(this.#encodeReturnCode(RETURN_CODES.SUCCESS, Buffer.alloc(32, 0)));
                    this.#draftEmits.push(txJson);
                }
                catch (e) {
                    console.error(e);
                    this.#sendToProc(this.#encodeReturnCode(RETURN_CODES.INTERNAL_ERROR));
                }
                break;
            case (MESSAGE_TYPES.TRUSTLINE):
                try {
                    // Decode trustline request info from the buf.
                    const trustlineInfo = this.#decodeTrustlineRequest(content);
                    // Get trustlines from the hook account.
                    let lines = await this.#hookAccount.xrplApi.getTrustlines(trustlineInfo.address, {
                        limit: 399,
                        peer: trustlineInfo.issuer
                    });
                    lines = lines.filter(l => l.currency === trustlineInfo.currency);
                    // Encode the trustline info to a buf and send to child process.
                    if (!lines || !lines.length)
                        this.#sendToProc(this.#encodeReturnCode(RETURN_CODES.NOT_FOUND));
                    else
                        this.#sendToProc(this.#encodeReturnCode(RETURN_CODES.SUCCESS, this.#encodeTrustLines(lines)));
                }
                catch (e) {
                    console.error(e);
                    this.#sendToProc(this.#encodeReturnCode(RETURN_CODES.INTERNAL_ERROR));
                }
                break;
            case (MESSAGE_TYPES.STATE_GET):
                try {
                    // Decode state get request info from the buf.
                    const stateGetInfo = this.#decodeStateGetRequest(content);
                    const value = await this.#stateManager.get(stateGetInfo.key);
                    this.#sendToProc(this.#encodeReturnCode(value, value));
                }
                catch (e) {
                    // Log the errors other than does not exist.
                    if (e.code !== STATE_ERROR_CODES.DOES_NOT_EXIST)
                        console.log(e);

                    let retCode = RETURN_CODES.INTERNAL_ERROR;
                    if (e.code === STATE_ERROR_CODES.DOES_NOT_EXIST)
                        retCode = RETURN_CODES.NOT_FOUND;
                    else if (e.code === STATE_ERROR_CODES.KEY_TOO_SMALL)
                        retCode = RETURN_CODES.UNDERFLOW;
                    else if (e.code === STATE_ERROR_CODES.KEY_TOO_BIG)
                        retCode = RETURN_CODES.OVERFLOW;

                    this.#sendToProc(this.#encodeReturnCode(retCode));
                }
                break;
            case (MESSAGE_TYPES.STATE_SET):
                try {
                    // Decode state set request info from the buf.
                    const stateSetInfo = this.#decodeStateSetRequest(content);
                    this.#stateManager.set(stateSetInfo.key, stateSetInfo.value);
                    this.#sendToProc(this.#encodeReturnCode(RETURN_CODES.SUCCESS));
                }
                catch (e) {
                    console.error(e);
                    let retCode = RETURN_CODES.INTERNAL_ERROR;
                    if (e.code === STATE_ERROR_CODES.KEY_TOO_BIG || e.code === STATE_ERROR_CODES.VALUE_TOO_BIG)
                        retCode = RETURN_CODES.OVERFLOW;
                    else if (e.code === STATE_ERROR_CODES.KEY_TOO_SMALL)
                        retCode = RETURN_CODES.UNDERFLOW;

                    this.#sendToProc(this.#encodeReturnCode(retCode));
                }
                break;
            case (MESSAGE_TYPES.ACCID):
                try {
                    const accountId = codec.decodeAccountID(content.toString());
                    this.#sendToProc(this.#encodeReturnCode(RETURN_CODES.SUCCESS, accountId));
                } catch (error) {
                    this.#sendToProc(this.#encodeReturnCode(RETURN_CODES.INTERNAL_ERROR));
                }
                break;
            case (MESSAGE_TYPES.SEQUENCE):
                try {
                    const value = await this.#hookAccount.getSequence();
                    this.#sendToProc(this.#encodeReturnCode(RETURN_CODES.SUCCESS, this.#encodeUint32(value)));
                } catch (error) {
                    this.#sendToProc(this.#encodeReturnCode(RETURN_CODES.INTERNAL_ERROR));
                }
                break;
            case (MESSAGE_TYPES.MINTED_TOKENS):
                try {
                    const value = this.#accountManager.getMintedTokensSeq();
                    this.#sendToProc(this.#encodeReturnCode(RETURN_CODES.SUCCESS, this.#encodeUint32(value)));
                } catch (error) {
                    this.#sendToProc(this.#encodeReturnCode(RETURN_CODES.INTERNAL_ERROR));
                }
                break;
            default:
                break;
        }
    }
}

module.exports = {
    TransactionManager
}
