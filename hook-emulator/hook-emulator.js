const fs = require('fs');
const { StateManager } = require('./state-manager');
const { TransactionManager } = require('./transaction-manager');
const { XrplApi } = require('./lib/xrpl-api');

const CONFIG_PATH = './hook-emulator.cfg';
const DB_PATH = './mb-xrpl.sqlite';
const HOOK_WRAPPER_PATH = './c_wrappers/hook-wrapper';

const RIPPLED_URL = "wss://s.altnet.rippletest.net:51233";

class HookEmulator {
    #rippledServer = null;
    #hookAddress = null;
    #stateManager = null;
    #transactionManager = null;
    #xrplApi = null;

    constructor(rippledServer, hookAddress, hookWrapperPath, dbPath) {
        this.#rippledServer = rippledServer;
        this.#hookAddress = hookAddress;
        this.#stateManager = new StateManager(dbPath);
        this.#transactionManager = new TransactionManager(hookWrapperPath, this.#stateManager);
        this.#xrplApi = new XrplApi(rippledServer);
    }

    async init() {
        console.log("Starting hook emulator.");

        await this.#xrplApi.connect();
        await this.#xrplApi.subscribeToAddress(this.#hookAddress, async (tx, error) => {
            if (error)
                console.error(error);
            const resCode = await this.#transactionManager.processTransaction(tx).catch(console.error);
            if (resCode)
                console.log(resCode);
        });

        console.log(`Listening to hook address ${this.#hookAddress} on ${this.#rippledServer}`);
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
