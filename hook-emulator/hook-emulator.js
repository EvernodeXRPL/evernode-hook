const fs = require('fs');
const process = require('process');
const { StateManager } = require('./lib/state-manager');
const { AccountManager } = require('./lib/account-manager');
const { TransactionManager } = require('./lib/transaction-manager');
const evernode = require('evernode-js-client');
const { IndexManager } = require('./lib/index-manager');

const DATA_DIR = process.env.DATA_DIR || __dirname;
const RIPPLED_URL = process.env.RIPPLED_URL || "wss://xls20-sandbox.rippletest.net:51233";

const CONFIG_PATH = DATA_DIR + '/hook-emulator.cfg';
const DB_PATH = DATA_DIR + '/hook-emulator.sqlite';
const HOOK_WRAPPER_PATH = DATA_DIR + '/c_wrappers/hook-wrapper';
const FIREBASE_SEC_KEY_PATH = DATA_DIR + '/sec/firebase-sa-key.json';

/**
 * Hook emulator listens to the transactions on the hook account and pass them through transaction manager to do the hook logic execution.
 */
class HookEmulator {
    #indexManager = null;
    #stateManager = null;
    #accountManager = null;
    #transactionManager = null;
    #xrplApi = null;
    #xrplAcc = null;

    constructor(rippledServer, hookWrapperPath, dbPath, hookAddress, hookSecret) {
        this.#indexManager = new IndexManager();
        this.#stateManager = new StateManager(dbPath, this.#indexManager);
        this.#accountManager = new AccountManager();
        this.#xrplApi = new evernode.XrplApi(rippledServer);
        evernode.Defaults.set({
            hookAddress: hookAddress,
            rippledServer: rippledServer,
            xrplApi: this.#xrplApi
        })
        this.#xrplAcc = new evernode.XrplAccount(hookAddress, hookSecret);
        this.#transactionManager = new TransactionManager(this.#xrplAcc, hookWrapperPath, this.#stateManager, this.#accountManager);
    }

    async init(firebaseSecKeyPath) {
        console.log("Starting hook emulator.");
        await this.#xrplApi.connect();

        await this.#indexManager.init(firebaseSecKeyPath);
        await this.#stateManager.init();
        this.#accountManager.init();

        // Handle the transaction when payment transaction is received.
        // Note - This event will only receive the incoming payment transactions to the hook.
        // We currently only need hook to execute incoming payment transactions. 
        this.#xrplAcc.on(evernode.XrplApiEvents.PAYMENT, async (tx, error) => await this.#handleTransaction(tx, error));

        // Subscribe for the transactions
        await this.#xrplAcc.subscribe();
        console.log(`Listening to hook address ${evernode.Defaults.get().hookAddress} on ${evernode.Defaults.get().rippledServer}`);

    }

    async #handleTransaction(tx, error) {
        // If transaction is not a success, log the error and skip.
        if (error) {
            console.error(error);
            return;
        }

        // Process the transaction and log the result code.
        const resCode = await this.#transactionManager.processTransaction(tx).catch(console.error);
        if (resCode)
            console.log(resCode);
    }
}

async function main() {
    let config = { hookAddress: "", hookSecret: "" };

    // Check if config file exists, write otherwise.
    if (!fs.existsSync(CONFIG_PATH))
        fs.writeFileSync(CONFIG_PATH, JSON.stringify(config, null, 4));
    else
        config = JSON.parse(fs.readFileSync(CONFIG_PATH).toString());

    // If hook address is empty, skip the execution.
    if (!config.hookAddress || !config.hookSecret) {
        console.error(`${CONFIG_PATH} not found, populate the config and restart the program.`);
        return;
    }

    // Start the emulator.
    // Note - Hook wrapper path is the path to the hook wrapper binary.
    const emulator = new HookEmulator(RIPPLED_URL, HOOK_WRAPPER_PATH, DB_PATH, config.hookAddress, config.hookSecret);
    if (fs.existsSync(FIREBASE_SEC_KEY_PATH))
        await emulator.init(FIREBASE_SEC_KEY_PATH);
    else {
        console.error(`${FIREBASE_SEC_KEY_PATH} not found, place the json file in the location.`);
        return;
    }
}

main().catch(console.error);
