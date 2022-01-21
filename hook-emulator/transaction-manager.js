const { exec } = require("child_process");
const codec = require('ripple-address-codec');
const { XflHelpers } = require('./lib/xfl-helpers');

const TX_WAIT_TIMEOUT = 10000;

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

    constructor(xrplAcc, hookWrapperPath, stateManager) {
        this.#xrplAcc = xrplAcc;
        this.#hookWrapperPath = hookWrapperPath;
        this.#stateManager = stateManager;
    }

    processTransaction(transaction) {
        const accountBuf = codec.decodeAccountID(transaction.Account);
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
        const destinationBuf = codec.decodeAccountID(transaction.Destination);
        const memos = transaction.Memos ? transaction.Memos : [];
        let memoCountBuf = Buffer.allocUnsafe(1);
        memoCountBuf.writeUInt8(memos.length);

        let memosBuf = Buffer.from([]);
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

            memosBuf = Buffer.concat([memosBuf, typeLenBuf, typeBuf, formatLenBuf, formatBuf, dataLenBuf, dataBuf]);
        }
        const ledgerHashBuf = Buffer.from(transaction.LedgerHash, 'hex');
        let ledgerIndexBuf = Buffer.allocUnsafe(8);
        ledgerIndexBuf.writeBigUInt64BE(BigInt(transaction.LedgerIndex));

        const txBuf = Buffer.concat([accountBuf, isXrpBuf, amountBuf, destinationBuf, memoCountBuf, memosBuf, ledgerHashBuf, ledgerIndexBuf]);
        const txLenBuf = Buffer.allocUnsafe(4);
        txLenBuf.writeUInt32BE(txBuf.length);

        return new Promise((resolve, reject) => {
            let completed = false;

            const failTimeout = setTimeout(() => {
                this.#rollbackTransaction();
                reject(ErrorCodes.TIMEOUT);
                child.stdin.end();
                completed = true;
            }, TX_WAIT_TIMEOUT);

            const child = exec(this.#hookWrapperPath);

            // Getting the exit code.
            child.on('close', (code) => {
                if (!completed) {
                    clearTimeout(failTimeout);
                    if (code == 0)
                        resolve("SUCCESS");
                    else {
                        this.#rollbackTransaction();
                        reject(ErrorCodes.TX_ERROR);
                    }
                    child.stdin.end();
                    completed = true;
                }
            });

            child.stdout.setEncoding('binary');
            child.stdout.on('data', async (data) => {
                if (!completed) {
                    const messageBuf = Buffer.from(data, "binary");
                    let offset = 0;
                    while (offset < messageBuf.length) {
                        const msgLen = messageBuf.readUInt32BE(offset);
                        offset += 4;
                        const msg = messageBuf.slice(offset, offset + msgLen);
                        offset += msgLen;
                        await this.#handleMessage(child, msg);
                    }
                }
            });

            child.stderr.setEncoding('utf8');
            child.stderr.on('data', async (data) => {
                if (!completed) {
                    console.log(data);
                }
            });

            child.stdin.setEncoding('binary');
            child.stdin.write(Buffer.concat([txLenBuf, txBuf]));
        });
    }

    #rollbackTransaction() {

    }

    async #handleMessage(proc, messageBuf) {
        const type = messageBuf.readUInt8(0);
        const content = messageBuf.slice(1);
        if (type === MESSAGE_TYPES.TRACE)
            console.log(content.toString());
        else if (type === MESSAGE_TYPES.EMIT) {
            let offset = 0;
            const account = codec.encodeAccountID(content.slice(offset, offset + 20));
            offset += 20;
            const isXrp = (content.readUInt8(offset) === 1);
            offset++;
            let amount;
            if (isXrp) {
                const xfl = content.readBigUInt64BE(offset);
                offset += 8;
                amount = XflHelpers.toString(xfl);
            }
            else {
                const issuer = codec.encodeAccountID(content.slice(offset, offset + 20));
                offset += 20;
                const currency = content.slice(offset, offset + 3).toString();
                offset += 3;
                const xfl = content.readBigUInt64BE(offset);
                offset += 8;
                const value = XflHelpers.toString(xfl);
                amount = {
                    issuer: issuer,
                    currency: currency,
                    value: value
                }
            }
            const destination = codec.encodeAccountID(content.slice(offset, offset + 20));
            offset += 20;
            const memoCount = content.readUInt8(offset);
            offset++;
            let memos = [];
            for (let i = 0; i < memoCount; i++) {
                const typeLen = content.readUInt8(offset);
                offset++;
                const type = content.slice(offset, offset + typeLen).toString();
                offset += typeLen;
                const formatLen = content.readUInt8(offset);
                offset++;
                const format = content.slice(offset, offset + formatLen).toString();
                offset += formatLen;
                const dataLen = content.readUInt8(offset);
                offset++;
                const data = content.slice(offset, offset + dataLen).toString(format === 'hex' ? 'hex' : 'utf8');
                offset += dataLen;
                memos.push({
                    type: type,
                    format: format,
                    data: data
                });
            }
            const ledgerHash = content.slice(offset, offset + 32).toString('hex');
            offset += 32;
            const ledgerIndex = Number(content.readBigUInt64BE(offset));
            const tx = {
                Account: account,
                Amount: amount,
                Destination: destination,
                Memos: memos,
                LedgerHash: ledgerHash,
                LedgerIndex: ledgerIndex
            }
            console.log("Received emit transaction : ", tx);
        }
        else if (type === MESSAGE_TYPES.KEYLET) {
            let offset = 0;
            const issuer = codec.encodeAccountID(content.slice(offset, offset + 20));
            offset += 20;
            const currency = content.slice(offset, offset + 3).toString();
            const lines = await this.#xrplAcc.getTrustLines(currency, issuer);
            let linesBuf = Buffer.from([]);
            if (lines && lines.length) {
                const issuerBuf = codec.decodeAccountID(lines[0].account);
                const currencyBuf = Buffer.from(lines[0].currency);
                let balanceBuf = Buffer.allocUnsafe(8);
                balanceBuf.writeBigUInt64BE(XflHelpers.getXfl(lines[0].balance));
                let limitBuf = Buffer.allocUnsafe(8);
                limitBuf.writeBigUInt64BE(XflHelpers.getXfl(lines[0].limit));
                linesBuf = Buffer.concat([issuerBuf, currencyBuf, balanceBuf, limitBuf]);
            }
            const linesLenBuf = Buffer.allocUnsafe(4);
            linesLenBuf.writeUInt32BE(linesBuf.length);

            proc.stdin.write(Buffer.concat([linesLenBuf, linesBuf]));
        }
        else if (type === MESSAGE_TYPES.STATEGET) {
            this.#stateManager.get("key");
        }
        else if (type === MESSAGE_TYPES.STATESET) {
            this.#stateManager.set("key", "value");
        }
    }
}

module.exports = {
    TransactionManager
}
