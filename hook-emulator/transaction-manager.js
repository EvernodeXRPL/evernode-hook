const { exec } = require("child_process");

const TX_WAIT_TIMEOUT = 3000;

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
            let completed = false;

            const failTimeout = setTimeout(() => {
                this.#rollbackTransaction();
                reject(ErrorCodes.TIMEOUT);
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
                    completed = true;
                }
            });

            child.stdout.setEncoding('utf8');
            child.stdout.on('data', (data) => {
                if (!completed) {
                    const lines = data.split('\n');
                    for (const line of lines)
                        this.#handleMessage(line);
                }
            });

            child.stdin.setEncoding('utf-8');
            child.stdin.write(JSON.stringify(transaction));
            child.stdin.write('\n');

            child.stdin.end();
        });
    }

    #rollbackTransaction() {

    }

    #handleMessage(message) {
        if (message.startsWith("**TRACE**"))
            console.log(message.substring(9));
        else if (message.startsWith("**EMIT**")) {
            console.log("Received emit transaction : ", message.substring(8));
        }
        else if (message.startsWith("**KEYLET**")) {
            console.log("Received keylet request");
        }
        else if (message.startsWith("**STATEGET**")) {
            this.#stateManager.get("key");
        }
        else if (message.startsWith("**STATESET**")) {
            this.#stateManager.set("key", "value");
        }
    }
}

module.exports = {
    TransactionManager
}
