const fs = require('fs');
const process = require('process');
const evernode = require('evernode-js-client');
const logger = require('./lib/logger');
const { StateManager } = require('./lib/state-manager');
const { AccountManager } = require('./lib/account-manager');
const { TransactionManager } = require('./lib/transaction-manager');
const { FirestoreManager } = require('./lib/firestore-manager');

const DATA_DIR = process.env.DATA_DIR || __dirname;
const RIPPLED_URL = process.env.RIPPLED_URL || "wss://xls20-sandbox.rippletest.net:51233";
const MODE = process.env.MODE || 'dev';
const FILE_LOG_ENABLED = process.env.FILE_LOG === '1';

const LOG_FILE_SUFFIX = '_hook-emulator.log';
const DB_FILE_SUFFIX = '_hook-emulator.sqlite';
const ACC_DATA_FILE_SUFFIX = '_hook-root.json';
const HOOK_WRAPPER_PATH = DATA_DIR + '/c-wrappers/hook-wrapper';
const FIREBASE_SEC_KEY_PATH = DATA_DIR + '/sec/firebase-sa-key.json';
const BETA_STATE_INDEX = ""; // This constant will be populated when beta firebase project is created.

/**
 * Hook emulator listens to the transactions on the hook account and pass them through transaction manager to do the hook logic execution.
 */
class HookEmulator {
    #firestoreManager = null;
    #stateManager = null;
    #accountManager = null;
    #transactionManager = null;
    #xrplApi = null;
    #xrplAcc = null;

    constructor(rippledServer, hookWrapperPath, dbFilePath, accountDataFilePath, hookAddress, hookSecret, stateIndexId = null) {
        this.#xrplApi = new evernode.XrplApi(rippledServer);
        evernode.Defaults.set({
            registryAddress: hookAddress,
            rippledServer: rippledServer,
            xrplApi: this.#xrplApi
        })
        this.#xrplAcc = new evernode.XrplAccount(hookAddress, hookSecret);
        this.#firestoreManager = new FirestoreManager(stateIndexId ? { stateIndexId: stateIndexId } : {});
        this.#stateManager = new StateManager(dbFilePath, this.#firestoreManager);
        this.#accountManager = new AccountManager(accountDataFilePath);
        this.#transactionManager = new TransactionManager(this.#xrplAcc, hookWrapperPath, this.#stateManager, this.#accountManager);
    }

    async init(firebaseSecKeyPath) {
        console.log("Starting hook emulator.");
        await this.#xrplApi.connect();

        await this.#firestoreManager.authorize(firebaseSecKeyPath);
        await this.#stateManager.init();
        this.#accountManager.init();

        // Handle the transaction when payment transaction is received.
        // Note - This event will only receive the incoming payment transactions to the hook.
        // We currently only need hook to execute incoming payment transactions. 
        this.#xrplAcc.on(evernode.XrplApiEvents.PAYMENT, async (tx, error) => await this.#handleTransaction(tx, error));

        // Subscribe for the transactions
        await this.#xrplAcc.subscribe();
        console.log(`Listening to hook address ${evernode.Defaults.get().registryAddress} on ${evernode.Defaults.get().rippledServer}`);

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
    if (process.argv.length !== 4)
        throw "Invalid args. Hook Address and Secret are required as arguments.";

    let hookAddress = process.argv[2];
    let hookSecret = process.argv[3];

    // If hook address is empty, skip the execution.
    if (!hookAddress || !hookSecret)
        throw "Invalid args. Hook Address and Secret are required as arguments.";

    // Setup the logging.
    const logFilePath = `${DATA_DIR}/log/${hookAddress}${LOG_FILE_SUFFIX}`;
    logger.init(logFilePath, FILE_LOG_ENABLED);

    // Start the emulator.
    // Note - Hook wrapper path is the path to the hook wrapper binary.
    // Setup beta state index if mode is beta.
    const dbFilePath = `${DATA_DIR}/${hookAddress}${DB_FILE_SUFFIX}`;
    const accDataFilePath = `${DATA_DIR}/${hookAddress}${ACC_DATA_FILE_SUFFIX}`;
    const emulator = new HookEmulator(RIPPLED_URL, HOOK_WRAPPER_PATH, dbFilePath, accDataFilePath, hookAddress, hookSecret, MODE === 'beta' ? BETA_STATE_INDEX : null);
    if (fs.existsSync(FIREBASE_SEC_KEY_PATH))
        await emulator.init(FIREBASE_SEC_KEY_PATH);
    else {
        console.error(`${FIREBASE_SEC_KEY_PATH} not found, place the json file in the location.`);
        return;
    }
}

main().catch(console.error);
