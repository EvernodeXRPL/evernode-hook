const { exec } = require("child_process");
const codec = require('ripple-address-codec');
const rippleCodec = require('ripple-binary-codec');
const { XflHelpers } = require('evernode-js-client');

const TX_WAIT_TIMEOUT = 5000;

const ErrorCodes = {
    TIMEOUT: 'TIMEOUT',
    TX_ERROR: 'TX_ERROR'
}

const MESSAGE_TYPES = {
    TRACE: 0,
    EMIT: 1,
    KEYLET: 2,
    STATE_GET: 3,
    STATE_SET: 4
};

const XRP_CURRENCY = 'XRP';

// ------------------ Message formats ------------------
// Message protocol - <data len(4)><data>
// -----------------------------------------------------

// --------------- Message data formats ----------------

// ''''' JS --> C_WRAPPER (Write to STDIN from JS) '''''
// Transaction origin - <hookid(20)><account(20)><1 for xrp and 0 for iou(1)><[XRP: amount in buf(8)XFL][IOU: <issuer(20)><currency(3)><amount in buf(8)XFL>]><destination(20)><memo count(1)><[<TypeLen(1)><MemoType(20)><FormatLen(1)><MemoFormat(20)><DataLen(1)><MemoData(128)>]><ledger_hash(32)><ledger_index(8)>
// Keylet response - <issuer(20)><currency(3)><balance(8)XFL><limit(8)XFL>
// State get response - <value(128)>
// '''''''''''''''''''''''''''''''''''''''''''''''''''''

// ' C_WRAPPER --> JS (Write to STDOUT from C_WRAPPER) '
// Trace - <TYPE:TRACE(1)><trace message>
// Transaction emit - <TYPE:EMIT(1)><prepared transaction buf>
// Keylet request - <TYPE:KEYLET(1)><issuer(20)><currency(3)>
// State set request - <TYPE:STATE_GET(1)><key(32)><value(128)>
// State get request - <TYPE:STATE_SET(1)><key(32)>
// '''''''''''''''''''''''''''''''''''''''''''''''''''''
// -----------------------------------------------------

/**
 * Transaction manager handles the transactions.
 * Executes the hook as a child process and commmunicate with child process using STDIN and STDOUT.
 */
class TransactionManager {
    #hookWrapperPath = null;
    #stateManager = null;
    #hookAccount = null;
    #hookProcess = null;
    #draftEmits = null;
    #pendingTransactions = null;
    #queueProcessorActive = null;

    constructor(hookAccount, hookWrapperPath, stateManager) {
        this.#hookAccount = hookAccount;
        this.#hookWrapperPath = hookWrapperPath;
        this.#stateManager = stateManager;
        this.#pendingTransactions = [];
        this.#queueProcessorActive = false;
        this.#initDrafts();
    }

    async init() {
        await this.#stateManager.init();
    }

    #initDrafts() {
        this.#draftEmits = [];
    }

    #encodeTransaction(transaction) {
        // Check whether transaction is a xrp transaction  or not.
        const isXrp = (typeof transaction.Amount === 'string');

        // Pre allocate a buffer to propulate transaction info. allocUnsafe since it's faster and the whole buffer will be allocated in the later part.
        let txBuf = Buffer.allocUnsafe(42 + (isXrp ? 8 : 31))

        let offset = 0;
        // Get origin account id from r-address and populate (20-bytes).
        codec.decodeAccountID(transaction.Account).copy(txBuf, offset);
        offset += 20;

        // Amount buf -> if transaction is a xrp transaction <amount|xfl(8-bytes)> otherwise <issuer(20-bytes)><currency(30-bytes)><value|xfl(8-bytes)>
        if (isXrp) {
            txBuf.writeUInt8(1, offset++);
            // Convert amount to xfl and populate.
            txBuf.writeBigInt64BE(XflHelpers.getXfl(transaction.Amount), offset);
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
        // Return a buffer if there're trustlines, Otherwise return an empty buf.
        if (trustLines && trustLines.length) {
            // Pre allocate buffer to populate trustline data.
            let buf = Buffer.allocUnsafe(39)

            let offset = 0;
            // Get account id from r-address and populate (20-bytes).
            codec.decodeAccountID(trustLines[0].account).copy(buf, offset);
            offset += 20;
            // Populate curreny (3-bytes).
            Buffer.from(trustLines[0].currency).copy(buf, offset);
            offset += 3;
            // Convert balance to xfl and populate (8-bytes).
            buf.writeBigInt64BE(XflHelpers.getXfl(trustLines[0].balance), offset);
            offset += 8;
            // Convert limit to xfl and populate (8-bytes).
            buf.writeBigInt64BE(XflHelpers.getXfl(trustLines[0].limit), offset);
            offset += 8;

            return buf;
        }
        else
            return Buffer.from([]);
    }

    #encodeStateValue(value) {
        return value ? Buffer.from(value, 'hex') : Buffer.from([]);
    }

    #decodeKeyletRequest(requestBuf) {
        let request = {};

        let offset = 0;
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
        request.key = requestBuf.slice(0, 32).toString('hex').toUpperCase();

        return request;
    }

    #decodeStateSetRequest(requestBuf) {
        let request = {};

        let offset = 0;
        // Get the key (32-bytes).
        request.key = requestBuf.slice(offset, offset + 32).toString('hex').toUpperCase();
        offset += 32;

        // Get the value (128-bytes).
        request.value = requestBuf.slice(offset, offset + 128).toString('hex').toUpperCase();

        return request;
    }

    #sendToProc(data) {
        // Append data length (4-bytes) to the message and write to STDIN.
        const lenBuf = Buffer.allocUnsafe(4);
        lenBuf.writeUInt32BE(data.length);
        this.#hookProcess.stdin.write(Buffer.concat([lenBuf, data]));
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
                await this.#persistTransaction();
                task.resolve(ret);
            }
            catch (e) {
                this.#rollbackTransaction();
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
            const failTimeout = setTimeout(async () => {
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
        for (const transaction of this.#draftEmits) {
            let txJson = rippleCodec.decode(transaction.toString('hex'));
            delete txJson.SigningPubKey;
            const signedTx = this.#hookAccount.wallet.sign(txJson);
            await this.#hookAccount.submitTransactionBlob(signedTx.tx_blob);
        }
        await this.#stateManager.persist();
    }

    #rollbackTransaction() {
        this.#stateManager.rollback();
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
                this.#draftEmits.push(content);
                break;
            case (MESSAGE_TYPES.KEYLET):
                // Decode keylet request info from the buf.
                const keyletInfo = this.#decodeKeyletRequest(content);
                // Get trustlines from the hook account.
                const lines = await this.#hookAccount.getTrustLines(keyletInfo.currency, keyletInfo.issuer);
                // Encode the trustline info to a buf and send to child process.
                this.#sendToProc(this.#encodeTrustLines(lines));
                break;
            case (MESSAGE_TYPES.STATE_GET):
                // Decode state get request info from the buf.
                const stateGetInfo = this.#decodeStateGetRequest(content);
                const value = await this.#stateManager.get(stateGetInfo.key);
                this.#sendToProc(this.#encodeStateValue(value));
                break;
            case (MESSAGE_TYPES.STATE_SET):
                // Decode state set request info from the buf.
                const stateSetInfo = this.#decodeStateSetRequest(content);
                this.#stateManager.set(stateSetInfo.key, stateSetInfo.value);
                break;
            default:
                break;
        }
    }
}

module.exports = {
    TransactionManager
}
