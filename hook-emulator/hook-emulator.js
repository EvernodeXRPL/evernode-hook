const fs = require('fs');
const { StateManager } = require('./state-manager');
const { TransactionManager } = require('./transaction-manager');
const evernode = require('evernode-js-client');

const CONFIG_PATH = './hook-emulator.cfg';
const DB_PATH = './mb-xrpl.sqlite';
const HOOK_WRAPPER_PATH = './c_wrappers/hook-wrapper';

const RIPPLED_URL = "wss://s.altnet.rippletest.net:51233";

class HookEmulator {
    #stateManager = null;
    #transactionManager = null;
    #xrplApi = null;
    #xrplAcc = null;

    constructor(rippledServer, hookAddress, hookWrapperPath, dbPath) {
        this.#stateManager = new StateManager(dbPath);
        this.#transactionManager = new TransactionManager(hookWrapperPath, this.#stateManager);
        this.#xrplApi = new evernode.XrplApi(rippledServer);
        evernode.Defaults.set({
            hookAddress: hookAddress,
            rippledServer: rippledServer,
            xrplApi: this.#xrplApi
        })
        this.#xrplAcc = new evernode.XrplAccount(hookAddress);
        this.#xrplAcc.on(evernode.XrplApiEvents.PAYMENT, async (tx, error) => await this.#handleTransaction(tx, error));
    }

    async init() {
        console.log("Starting hook emulator.");
        await this.#xrplApi.connect();
        await this.#xrplAcc.subscribe();
        console.log(`Listening to hook address ${evernode.Defaults.get().hookAddress} on ${evernode.Defaults.get().rippledServer}`);
    }

    async #handleTransaction(tx, error) {
        if (error)
            console.error(error);
        const resCode = await this.#transactionManager.processTransaction(tx).catch(console.error);
        if (resCode)
            console.log(resCode);
    }
}

async function main() {
    let config = { hookAddress: "" };

    if (!fs.existsSync(CONFIG_PATH))
        fs.writeFileSync(CONFIG_PATH, JSON.stringify(config, null, 4));
    else
        config = JSON.parse(fs.readFileSync(CONFIG_PATH).toString());

    if (!config.hookAddress) {
        console.log(`${CONFIG_PATH} not found, populate the config and restart the program.`);
        return;
    }

    const emulator = new HookEmulator(RIPPLED_URL, config.hookAddress, HOOK_WRAPPER_PATH, DB_PATH);
    await emulator.init();
}

main().catch(console.error);
