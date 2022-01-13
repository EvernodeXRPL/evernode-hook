const { spawn } = require("child_process");

const TX_WAIT_TIMEOUT = 10000;

const ErrorCodes = {
    TIMEOUT: 'TIMEOUT',
    TX_ERROR: 'TX_ERROR'
}

class TransactionManager {
    #hookWrapperPath = null;
    #stateManager = null;

    constructor(hookWrapperPath, stateManager) {
        this.#hookWrapperPath = hookWrapperPath;
        this.#stateManager = stateManager;
    }

    processTransaction(transaction) {
        return new Promise((resolve, reject) => {
            let child = spawn(this.#hookWrapperPath);

            child.stdout.on('data', (data) => {
                if (!data)
                    return;
                else
                    data = data.toString();

                if (data.startsWith("**TRACE**"))
                    console.log(data.substring(9));
                else if (data.startsWith("**EMIT**")) {
                    console.log("Received emit transaction : ", data.substring(8));
                }
                else if (data.startsWith("**KEYLET**")) {
                    console.log("Received keylet request");
                }
                else if (data.startsWith("**STATEGET**")) {
                    this.#stateManager.get("key");
                }
                else if (data.startsWith("**STATESET**")) {
                    this.#stateManager.set("key", "value");
                }
            });

            // Getting the exit code.
            child.on('close', function (code) {
                if (code == -1) {
                    this.#rollbackTransaction();
                    reject(ErrorCodes.TX_ERROR);
                    return;
                }
                else
                    resolve("SUCCESS");
            });

            child.stdin.setEncoding('utf-8');
            child.stdin.write(JSON.stringify(transaction));
            child.stdin.write('\n');

            child.stdin.end();

            setTimeout(() => {
                this.#rollbackTransaction();
                reject(ErrorCodes.TIMEOUT);
                return;
            }, TX_WAIT_TIMEOUT);
        });
    }

    #rollbackTransaction() {

    }
}

module.exports = {
    TransactionManager
}
