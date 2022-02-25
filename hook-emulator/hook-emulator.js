const fs = require('fs');
const process = require('process');
const { StateManager } = require('./lib/state-manager');
const { AccountManager } = require('./lib/account-manager');
const { TransactionManager } = require('./lib/transaction-manager');
const { ApiManager } = require('./lib/api-manager');
const evernode = require('evernode-js-client');

const CONFIG_PATH = './hook-emulator.cfg';
const DB_PATH = './hook-emulator.sqlite';
const HOOK_WRAPPER_PATH = './c_wrappers/hook-wrapper';
const API_PORT = 8080;

const RIPPLED_URL = "wss://xls20-sandbox.rippletest.net:51233";

/**
 * Hook emulator listens to the transactions on the hook account and pass them through transaction manager to do the hook logic execution.
 */
class HookEmulator {
    #stateManager = null;
    #accountManager = null;
    #transactionManager = null;
    #apiManager = null;
    #xrplApi = null;
    #xrplAcc = null;

    constructor(rippledServer, hookWrapperPath, dbPath, hookAddress, hookSecret, apiPort) {
        this.#stateManager = new StateManager(dbPath);
        this.#accountManager = new AccountManager();
        this.#xrplApi = new evernode.XrplApi(rippledServer);
        evernode.Defaults.set({
            hookAddress: hookAddress,
            rippledServer: rippledServer,
            xrplApi: this.#xrplApi
        })
        this.#xrplAcc = new evernode.XrplAccount(hookAddress, hookSecret);
        this.#transactionManager = new TransactionManager(this.#xrplAcc, hookWrapperPath, this.#stateManager, this.#accountManager);
        this.#apiManager = new ApiManager(apiPort)
    }

    async init() {
        console.log("Starting hook emulator.");
        await this.#xrplApi.connect();

        await this.#stateManager.init();
        this.#accountManager.init();
        this.#apiManager.init();

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
        console.log(`${CONFIG_PATH} not found, populate the config and restart the program.`);
        return;
    }

    // Start the emulator.
    // Note - Hook wrapper path is the path to the hook wrapper binary.
    const emulator = new HookEmulator(RIPPLED_URL, HOOK_WRAPPER_PATH, DB_PATH, config.hookAddress, config.hookSecret, (process.env.API_PORT || API_PORT));
    await emulator.init();
}

main().catch(console.error);
