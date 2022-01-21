const { exec } = require("child_process");
const codec = require('ripple-address-codec');
const { XflHelpers } = require('./lib/xfl-helpers');

const TX_WAIT_TIMEOUT = 5000;

const ErrorCodes = {
    TIMEOUT: 'TIMEOUT',
    TX_ERROR: 'TX_ERROR'
}

const MESSAGE_TYPES = {
    TRACE: 0,
    EMIT: 1,
    KEYLET: 2,
    STATEGET: 3,
    STATESET: 4
};

class TransactionManager {
    #hookWrapperPath = null;
    #stateManager = null;
    #xrplAcc = null;
    #hookProcess = null;

    constructor(xrplAcc, hookWrapperPath, stateManager) {
        this.#xrplAcc = xrplAcc;
        this.#hookWrapperPath = hookWrapperPath;
        this.#stateManager = stateManager;
    }

    #encodeTransaction(transaction) {
        let txBuf = codec.decodeAccountID(transaction.Account);

        const isXrp = (typeof transaction.Amount === 'string');
        let isXrpBuf = Buffer.allocUnsafe(1);
        let amountBuf;
        if (isXrp) {
            amountBuf = Buffer.allocUnsafe(8);
            isXrpBuf.writeUInt8(1);
            amountBuf.writeBigUInt64BE(XflHelpers.getXfl(transaction.Amount));
        }
        else {
            isXrpBuf.writeUInt8(0);
            let valueBuf = Buffer.allocUnsafe(8);
            valueBuf.writeBigUInt64BE(XflHelpers.getXfl(transaction.Amount.value))
            amountBuf = Buffer.concat([
                Buffer.from(codec.decodeAccountID(transaction.Amount.issuer)),
                Buffer.from(transaction.Amount.currency),
                valueBuf]);
        }

        const memos = transaction.Memos ? transaction.Memos : [];
        let memoCountBuf = Buffer.allocUnsafe(1);
        memoCountBuf.writeUInt8(memos.length);
        txBuf = Buffer.concat([txBuf, isXrpBuf, amountBuf, codec.decodeAccountID(transaction.Destination), memoCountBuf]);

        for (const memo of memos) {
            const typeBuf = Buffer.from(memo.type ? memo.type : []);
            let typeLenBuf = Buffer.allocUnsafe(1);
            typeLenBuf.writeUInt8(typeBuf.length);

            const formatBuf = Buffer.from(memo.format ? memo.format : []);
            let formatLenBuf = Buffer.allocUnsafe(1);
            formatLenBuf.writeUInt8(formatBuf.length);

            const dataBuf = memo.data ? Buffer.from(memo.data, memo.format === 'hex' ? 'hex' : 'utf8') : Buffer.from([]);
            let dataLenBuf = Buffer.allocUnsafe(1);
            dataLenBuf.writeUInt8(dataBuf.length);

            txBuf = Buffer.concat([txBuf, typeLenBuf, typeBuf, formatLenBuf, formatBuf, dataLenBuf, dataBuf]);
        }

        let ledgerIndexBuf = Buffer.allocUnsafe(8);
        ledgerIndexBuf.writeBigUInt64BE(BigInt(transaction.LedgerIndex));

        return Buffer.concat([txBuf, Buffer.from(transaction.LedgerHash, 'hex'), ledgerIndexBuf]);
    }

    #decodeTransaction(transactionBuf) {
        let transaction = {};
        let offset = 0;
        transaction.Account = codec.encodeAccountID(transactionBuf.slice(offset, offset + 20));
        offset += 20;

        if (transactionBuf.readUInt8(offset++) === 1) {
            transaction.Amount = XflHelpers.toString(transactionBuf.readBigUInt64BE(offset));
            offset += 8;
        }
        else {
            transaction.Amount = {};
            transaction.Amount.issuer = codec.encodeAccountID(transactionBuf.slice(offset, offset + 20));
            offset += 20;

            transaction.Amount.currency = transactionBuf.slice(offset, offset + 3).toString();
            offset += 3;

            transaction.Amount.value = XflHelpers.toString(transactionBuf.readBigUInt64BE(offset));
            offset += 8;
        }

        transaction.Destination = codec.encodeAccountID(transactionBuf.slice(offset, offset + 20));
        offset += 20;

        const memoCount = transactionBuf.readUInt8(offset);
        offset++;
        transaction.Memos = [];
        for (let i = 0; i < memoCount; i++) {
            const typeLen = transactionBuf.readUInt8(offset);
            offset++;
            const type = transactionBuf.slice(offset, offset + typeLen).toString();
            offset += typeLen;
            const formatLen = transactionBuf.readUInt8(offset);
            offset++;
            const format = transactionBuf.slice(offset, offset + formatLen).toString();
            offset += formatLen;
            const dataLen = transactionBuf.readUInt8(offset);
            offset++;
            const data = transactionBuf.slice(offset, offset + dataLen).toString(format === 'hex' ? 'hex' : 'utf8');
            offset += dataLen;
            transaction.Memos.push({
                type: type,
                format: format,
                data: data
            });
        }

        transaction.LedgerHash = transactionBuf.slice(offset, offset + 32).toString('hex');
        offset += 32;

        transaction.LedgerIndex = Number(transactionBuf.readBigUInt64BE(offset));

        return transaction;
    }

    #decodeKeyletRequest(requestBuf) {
        let request = {};
        let offset = 0;
        request.issuer = codec.encodeAccountID(requestBuf.slice(offset, offset + 20));
        offset += 20;

        request.currency = requestBuf.slice(offset, offset + 3).toString();

        return request;
    }

    #encodeTrustLines(trustLines) {
        if (trustLines && trustLines.length) {
            let buf = Buffer.concat([codec.decodeAccountID(trustLines[0].account), Buffer.from(trustLines[0].currency)]);

            let balanceBuf = Buffer.allocUnsafe(8);
            balanceBuf.writeBigUInt64BE(XflHelpers.getXfl(trustLines[0].balance));

            let limitBuf = Buffer.allocUnsafe(8);
            limitBuf.writeBigUInt64BE(XflHelpers.getXfl(trustLines[0].limit));

            return Buffer.concat([buf, balanceBuf, limitBuf]);
        }
        else
            return Buffer.from([]);
    }

    #sendToProc(data) {
        const lenBuf = Buffer.allocUnsafe(4);
        lenBuf.writeUInt32BE(data.length);
        this.#hookProcess.stdin.write(Buffer.concat([lenBuf, data]));
    }

    #terminateProc(success = false) {
        this.#hookProcess.stdin.end();
        this.#hookProcess = null;

        if (!success)
            this.#rollbackTransaction();
    }

    processTransaction(transaction) {
        const txBuf = this.#encodeTransaction(transaction);

        this.#hookProcess = exec(this.#hookWrapperPath);

        // Set stdin and stdout encoding to binary.
        this.#hookProcess.stdin.setEncoding('binary');
        this.#hookProcess.stdout.setEncoding('binary');
        this.#hookProcess.stderr.setEncoding('utf8');

        // Writing transaction to stdin.
        this.#sendToProc(txBuf);

        return new Promise((resolve, reject) => {
            let completed = false;

            const failTimeout = setTimeout(() => {
                reject(ErrorCodes.TIMEOUT);
                this.#terminateProc();
                completed = true;
            }, TX_WAIT_TIMEOUT);

            // Getting the exit code.
            this.#hookProcess.on('close', (code) => {
                if (!completed) {
                    clearTimeout(failTimeout);
                    if (code === 0)
                        resolve("SUCCESS");
                    else {
                        reject(ErrorCodes.TX_ERROR);
                    }
                    this.#terminateProc(code === 0);
                    completed = true;
                }
            });

            this.#hookProcess.stdout.on('data', async (data) => {
                if (!completed) {
                    const messageBuf = Buffer.from(data, "binary");

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

            this.#hookProcess.stderr.on('data', async (data) => {
                if (!completed) {
                    console.log(data);
                }
            });
        });
    }

    #rollbackTransaction() {

    }

    async #handleMessage(messageBuf) {
        const type = messageBuf.readUInt8(0);
        const content = messageBuf.slice(1);

        switch (type) {
            case (MESSAGE_TYPES.TRACE):
                console.log(content.toString());
                break;
            case (MESSAGE_TYPES.EMIT):
                const tx = this.#decodeTransaction(content);
                console.log("Received emit transaction : ", tx);
                break;
            case (MESSAGE_TYPES.KEYLET):
                const request = this.#decodeKeyletRequest(content);
                const lines = await this.#xrplAcc.getTrustLines(request.currency, request.issuer);
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
