const { exec } = require("child_process");
const codec = require('ripple-address-codec');
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

// ------------------ Message formats ------------------
// Message protocol - <data len(4)><data>
// -----------------------------------------------------

// --------------- Message data formats ----------------

// ''''' JS --> C_WRAPPER (Write to STDIN from JS) '''''
// Transaction origin - <hookid(20)><account(20)><1 for xrp and 0 for iou(1)><[XRP: amount in buf(8)XFL][IOU: <issuer(20)><currency(3)><amount in buf(8)XFL>]><destination(20)><memo count(1)><[<TypeLen(1)><MemoType(20)><FormatLen(1)><MemoFormat(20)><DataLen(1)><MemoData(128)>]><ledger_hash(32)><ledger_index(8)>
// Keylet response - <issuer(20)><currency(3)><balance(8)XFL><limit(8)XFL>
// '''''''''''''''''''''''''''''''''''''''''''''''''''''

// ' C_WRAPPER --> JS (Write to STDOUT from C_WRAPPER) '
// Trace - <TYPE:TRACE(1)><trace message>
// Transaction emit - <TYPE:EMIT(1)><account(20)><1 for xrp and 0 for iou(1)><[XRP: amount in buf(8)XFL][IOU: <issuer(20)><currency(3)><amount in buf(8)XFL>]><destination(20)><memo count(1)><[<TypeLen(1)><MemoType(20)><FormatLen(1)><MemoFormat(20)><DataLen(1)><MemoData(128)>]><ledger_hash(32)><ledger_index(8)>
// Keylet request - <TYPE:KEYLET(1)><issuer(20)><currency(3)>
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

    constructor(hookAccount, hookWrapperPath, stateManager) {
        this.#hookAccount = hookAccount;
        this.#hookWrapperPath = hookWrapperPath;
        this.#stateManager = stateManager;
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

    #decodeTransaction(transactionBuf) {
        let transaction = {};
        let offset = 0;

        // Get the origin account and encode the r-address (20-bytes).
        transaction.Account = codec.encodeAccountID(transactionBuf.slice(offset, offset + 20));
        offset += 20;

        // Get the amount -> if transaction is a xrp transaction <amount|xfl(8-bytes)> otherwise <issuer(20-bytes)><currency(30-bytes)><value|xfl(8-bytes)>
        if (transactionBuf.readUInt8(offset++) === 1) {
            // Get the amount and convert to float from xfl.
            transaction.Amount = XflHelpers.toString(transactionBuf.readBigInt64BE(offset));
            offset += 8;
        }
        else {
            transaction.Amount = {};

            // Get the issuer and encode the r-address.
            transaction.Amount.issuer = codec.encodeAccountID(transactionBuf.slice(offset, offset + 20));
            offset += 20;
            transaction.Amount.currency = transactionBuf.slice(offset, offset + 3).toString();
            offset += 3;
            // Get the amount value and convert to float from xfl.
            transaction.Amount.value = XflHelpers.toString(transactionBuf.readBigInt64BE(offset));
            offset += 8;
        }

        // Get the destination account and encode the r-address (20-bytes).
        transaction.Destination = codec.encodeAccountID(transactionBuf.slice(offset, offset + 20));
        offset += 20;

        // Get memo count (1-byte).
        const memoCount = transactionBuf.readUInt8(offset++);

        transaction.Memos = [];
        for (let i = 0; i < memoCount; i++) {
            // Get the memo type length (1-byte).
            const typeLen = transactionBuf.readUInt8(offset++);
            // Get the memo type ((typeLen)-bytes).
            const type = transactionBuf.slice(offset, offset + typeLen).toString();
            offset += typeLen;

            // Get the memo format length (1-byte).
            const formatLen = transactionBuf.readUInt8(offset++);
            // Get the memo format ((formatLen)-bytes).
            const format = transactionBuf.slice(offset, offset + formatLen).toString();
            offset += formatLen;

            // Get the memo data length (1-byte).
            const dataLen = transactionBuf.readUInt8(offset++);
            // Get the memo data ((dataLen)-bytes).
            // Convert data to hex if format is 'hex'.
            const data = transactionBuf.slice(offset, offset + dataLen).toString(format === 'hex' ? 'hex' : 'utf8');
            offset += dataLen;

            transaction.Memos.push({
                type: type,
                format: format,
                data: data
            });
        }

        // Get the ledger hash (32-bytes).
        transaction.LedgerHash = transactionBuf.slice(offset, offset + 32).toString('hex').toUpperCase();
        offset += 32;

        // Get the ledger index (8-bytes).
        transaction.LedgerIndex = Number(transactionBuf.readBigUInt64BE(offset));

        return transaction;
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

    #sendToProc(data) {
        // Append data length (4-bytes) to the message and write to STDIN.
        const lenBuf = Buffer.allocUnsafe(4);
        lenBuf.writeUInt32BE(data.length);
        this.#hookProcess.stdin.write(Buffer.concat([lenBuf, data]));
    }

    async #terminateProc(success = false) {
        this.#hookProcess.stdin.end();
        this.#hookProcess = null;

        if (success)
            await this.#persistTransaction();
        else
            this.#rollbackTransaction();
    }

    processTransaction(transaction) {
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

            const failTimeout = setTimeout(async () => {
                completed = true;
                await this.#terminateProc();
                reject(ErrorCodes.TIMEOUT);
            }, TX_WAIT_TIMEOUT);

            // Getting the exit code.
            this.#hookProcess.on('close', async (code) => {
                if (!completed) {
                    completed = true;
                    clearTimeout(failTimeout);
                    await this.#terminateProc(code === 0);
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

                        await this.#handleMessage(msg);
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
                // Decode the transaction data from the content buf.
                const tx = this.#decodeTransaction(content);
                console.log("Received emit transaction : ", tx);
                break;
            case (MESSAGE_TYPES.KEYLET):
                // Decode keylet request info from the buf.
                const request = this.#decodeKeyletRequest(content);
                // Get trustlines from the hook account.
                const lines = await this.#hookAccount.getTrustLines(request.currency, request.issuer);
                // Encode the trustline info to a buf and send to child process.
                this.#sendToProc(this.#encodeTrustLines(lines));
                break;
            case (MESSAGE_TYPES.STATEGET):
                this.#stateManager.get("key");
                break;
            case (MESSAGE_TYPES.STATESET):
                this.#stateManager.set("key", "value");
                break;
            default:
                break;
        }
    }
}

module.exports = {
    TransactionManager
}
