const fs = require('fs');
const process = require('process');
const path = require('path');
const codec = require('ripple-address-codec');
const evernode = require('evernode-js-client');
const { Buffer } = require('buffer');
const { StateManager } = require('./lib/state-manager');
const { AccountManager } = require('./lib/account-manager');
const { TransactionManager } = require('./lib/transaction-manager');
const { FirestoreManager } = require('./lib/firestore-manager');

const BETA_STATE_INDEX = ""; // This constant will be populated when beta firebase project is created.
const MIN_XRP = "1";
const INIT_MEMO_TYPE = "evnInitialize"; // This is kept only here as a constant, since we don't want to expose this event to public.
const INIT_MEMO_FORMAT = "hex";
const INIT_WAIT_TIMEOUT_SECS = 1000;

const RIPPLED_URL = process.env.RIPPLED_URL || "wss://xls20-sandbox.rippletest.net:51233";
const MODE = process.env.MODE || 'dev';

const DATA_DIR = process.env.DATA_DIR || __dirname;
const BIN_DIR = process.env.BIN_DIR || path.resolve(__dirname, 'dist');

const CONFIG_FILE = 'accounts.json';
const DB_FILE = 'hook-emulator.sqlite';
const HOOK_DATA_FILE = 'hook-root.json';
const HOOK_DATA_DIR = DATA_DIR + '/data';
const FIREBASE_SEC_KEY_PATH = DATA_DIR + '/sec/firebase-sa-key.json';
const HOOK_WRAPPER_PATH = BIN_DIR + '/hook-wrapper';

const HOOK_INITIALIZER = {
    address: 'rnzsYamjXaxAMg4JKp2VWeSWvvuvBaYAzX',
    secret: 'sn3jtrFQhMbXKeqND3yX1GuzVzioN'
}

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
    #completeHandlers = null;

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
        this.#completeHandlers = {};
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
        const resCode = await this.#transactionManager.processTransaction(tx).catch(e => {
            // Send errors to the subscribers if there's any.
            this.#handleComplete(tx.hash, null, e);
            console.error(e);
        });

        if (resCode) {
            // Send success res to the subscribers if there's any.
            this.#handleComplete(tx.hash, resCode);
            console.log(resCode);
        }
    }

    #handleComplete(txHash, ...params) {
        // Call the callback function with results and remove the it.
        if (this.#completeHandlers[txHash]) {
            this.#completeHandlers[txHash](...params);
            delete this.#completeHandlers[txHash];
        }
    }

    onComplete(txHash, callback) {
        this.#completeHandlers[txHash] = callback;
    }
}

async function initRegistryConfigs(config, configPath, emulator) {
    // Get issuer and foundation cold wallet account ids.
    let memoData = Buffer.allocUnsafe(40);
    codec.decodeAccountID(config.issuer.address).copy(memoData);
    codec.decodeAccountID(config.foundationColdWallet.address).copy(memoData, 20);

    const initAccount = new evernode.XrplAccount(HOOK_INITIALIZER.address, HOOK_INITIALIZER.secret);
    const res = await initAccount.makePayment(config.registry.address, MIN_XRP, 'XRP', null,
        [{ type: INIT_MEMO_TYPE, format: INIT_MEMO_FORMAT, data: memoData.toString('hex') }]);

    // If the transaction is success listen for the completion, if it's a success update the config file.
    return new Promise((resolve, reject) => {
        // Set a timeout of 30 seconds to reject in case if onComplete is delayed.
        setTimeout(() => {
            reject('Registry contract initialization wait timeout exceeded.');
            return;
        }, INIT_WAIT_TIMEOUT_SECS * 1000);

        if (res.code === 'tesSUCCESS') {
            emulator.onComplete(res.details.hash, (success, error) => {
                if (error) {
                    reject('Registry contract initialization failed.');
                    return;
                }
                else if (success) {
                    // Update the config initialized flag and write the config.
                    config.initialized = true;
                    fs.writeFileSync(configPath, JSON.stringify(config, null, 4));
                    resolve('Registry contract initialization success.');
                }

                reject('Unknown error occured while registry contract initialization.');
            });
        }
        else {
            reject(`Registry contract initialization transaction failed with ${res.code}.`);
        }
    });
}

async function main() {
    // Registry address is required as a command line param.
    if (process.argv.length != 3 || !process.argv[2]) {
        console.error('Registry address is required as a command line parameter.');
        return;
    }
    const registryAddress = process.argv[2];
    const configPath = path.resolve(HOOK_DATA_DIR, registryAddress, CONFIG_FILE);

    // If config doesn't exist, skip the execution.
    if (!fs.existsSync(configPath)) {
        console.error(`${configPath} not found, run the account setup tool and generate the accounts.`);
        return;
    }
    let config = JSON.parse(fs.readFileSync(configPath).toString());

    // If required files and config data doesn't exist, skip the execution.
    if (!config.registry || !config.registry.address || !config.registry.secret) {
        console.error('Registry account info not found, run the account setup tool and generate the accounts.');
        return;
    }
    else if (!config.issuer || !config.issuer.address) {
        console.error('Issuer account info not found, run the account setup tool and generate the accounts.');
        return;
    }
    else if (!config.foundationColdWallet || !config.foundationColdWallet.address || !config.foundationColdWallet.secret) {
        console.error('Foundation cold wallet info not found, run the account setup tool and generate the accounts.');
        return;
    }
    else if (!fs.existsSync(HOOK_WRAPPER_PATH)) {
        console.error(`${HOOK_WRAPPER_PATH} does not exist.`);
        return;
    }
    else if (!fs.existsSync(FIREBASE_SEC_KEY_PATH)) {
        console.error(`${FIREBASE_SEC_KEY_PATH} not found, place the json file in the location.`);
        return;
    }

    // Start the emulator.
    // Note - Hook wrapper path is the path to the hook wrapper binary.
    // Setup beta state index if mode is beta.
    const dbPath = path.resolve(HOOK_DATA_DIR, registryAddress, DB_FILE);
    const regDataPath = path.resolve(HOOK_DATA_DIR, registryAddress, HOOK_DATA_FILE)
    const emulator = new HookEmulator(RIPPLED_URL, HOOK_WRAPPER_PATH, dbPath, regDataPath, config.registry.address, config.registry.secret, MODE === 'beta' ? BETA_STATE_INDEX : null);
    await emulator.init(FIREBASE_SEC_KEY_PATH);

    // Send the config init transaction to the registry account.
    if (!config.initialized) {
        console.log('Sending registry contract initialization transation.');
        const res = await initRegistryConfigs(config, configPath, emulator).catch(console.error);
        if (res)
            console.log(res);
    }
}

main().catch(console.error);
