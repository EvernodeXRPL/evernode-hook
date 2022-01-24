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
        const isXrp = (typeof transaction.Amount === 'string');

        let txBuf = Buffer.allocUnsafe(42 + (isXrp ? 8 : 31))

        let offset = 0;
        codec.decodeAccountID(transaction.Account).copy(txBuf, offset);
        offset += 20;

        if (isXrp) {
            txBuf.writeUInt8(1, offset++);
            txBuf.writeBigInt64BE(XflHelpers.getXfl(transaction.Amount), offset);
            offset += 8;
        }
        else {
            txBuf.writeUInt8(0, offset++);
            codec.decodeAccountID(transaction.Amount.issuer).copy(txBuf, offset);
            offset += 20;
            Buffer.from(transaction.Amount.currency).copy(txBuf, offset);
            offset += 3;
            txBuf.writeBigInt64BE(XflHelpers.getXfl(transaction.Amount.value), offset);
            offset += 8;
        }

        codec.decodeAccountID(transaction.Destination).copy(txBuf, offset);
        offset += 20;

        const memos = transaction.Memos ? transaction.Memos : [];
        txBuf.writeUInt8(memos.length, offset++);

        for (const memo of memos) {
            const typeBuf = Buffer.from(memo.type ? memo.type : []);
            const formatBuf = Buffer.from(memo.format ? memo.format : []);
            const dataBuf = memo.data ? Buffer.from(memo.data, memo.format === 'hex' ? 'hex' : 'utf8') : Buffer.from([]);

            let memoBuf = Buffer.allocUnsafe(3 + typeBuf.length + formatBuf.length + dataBuf.length);

            let memoBufOffset = 0;
            memoBuf.writeUInt8(typeBuf.length, memoBufOffset++);
            typeBuf.copy(memoBuf, memoBufOffset);
            memoBufOffset += typeBuf.length;

            memoBuf.writeUInt8(formatBuf.length, memoBufOffset++);
            formatBuf.copy(memoBuf, memoBufOffset);
            memoBufOffset += formatBuf.length;

            memoBuf.writeUInt8(dataBuf.length, memoBufOffset++);
            dataBuf.copy(memoBuf, memoBufOffset);
            memoBufOffset += dataBuf.length;

            txBuf = Buffer.concat([txBuf, memoBuf]);
        }

        let returnBuf = Buffer.allocUnsafe(txBuf.length + 40);

        offset = 0;
        txBuf.copy(returnBuf, offset);
        offset += txBuf.length;
        Buffer.from(transaction.LedgerHash, 'hex').copy(returnBuf, offset);
        offset += 32;
        returnBuf.writeBigUInt64BE(BigInt(transaction.LedgerIndex), offset);

        return returnBuf;
    }

    #decodeTransaction(transactionBuf) {
        let transaction = {};
        let offset = 0;
        transaction.Account = codec.encodeAccountID(transactionBuf.slice(offset, offset + 20));
        offset += 20;

        if (transactionBuf.readUInt8(offset++) === 1) {
            transaction.Amount = XflHelpers.toString(transactionBuf.readBigInt64BE(offset));
            offset += 8;
        }
        else {
            transaction.Amount = {};
            transaction.Amount.issuer = codec.encodeAccountID(transactionBuf.slice(offset, offset + 20));
            offset += 20;

            transaction.Amount.currency = transactionBuf.slice(offset, offset + 3).toString();
            offset += 3;

            transaction.Amount.value = XflHelpers.toString(transactionBuf.readBigInt64BE(offset));
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

        transaction.LedgerHash = transactionBuf.slice(offset, offset + 32).toString('hex').toUpperCase();
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
            let buf = Buffer.allocUnsafe(39)

            let offset = 0;
            codec.decodeAccountID(trustLines[0].account).copy(buf, offset);
            offset += 20;
            Buffer.from(trustLines[0].currency).copy(buf, offset);
            offset += 3;

            buf.writeBigInt64BE(XflHelpers.getXfl(trustLines[0].balance), offset);
            offset += 8;
            buf.writeBigInt64BE(XflHelpers.getXfl(trustLines[0].limit), offset);
            offset += 8;

            return buf;
        }
        else
            return Buffer.from([]);
    }

    #sendToProc(data) {
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
        let txBuf = this.#encodeTransaction(transaction);

        // Append hook account id.
        txBuf = Buffer.concat([codec.decodeAccountID(this.#hookAccount.address), txBuf]);

        this.#hookProcess = exec(this.#hookWrapperPath);

        // Set stdin and stdout encoding to binary.
        this.#hookProcess.stdin.setEncoding('binary');
        this.#hookProcess.stdout.setEncoding('binary');
        this.#hookProcess.stderr.setEncoding('utf8');

        // Writing transaction to stdin.
        this.#sendToProc(txBuf);

        return new Promise((resolve, reject) => {
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

    async #persistTransaction() {
        await this.#stateManager.persist();
    }

    #rollbackTransaction() {
        this.#stateManager.rollback();
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
                const lines = await this.#hookAccount.getTrustLines(request.currency, request.issuer);
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
