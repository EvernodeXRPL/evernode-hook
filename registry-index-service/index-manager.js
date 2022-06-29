/**
 * This service helps to update the Evernode Firestore index, when registry update is occured.
 * Here, the transactions are collected to a golbal array (FIFO Queue) and sequentially process them batch-wise.
 * Interval based scheduler is running to process the transactions.
 *
 * NOTE : Firestore update is not a batch write for the moment.
 */
const fs = require('fs');
const process = require('process');
const path = require('path');
const codec = require('ripple-address-codec');
const {
    XrplApi, XrplAccount, StateHelpers,
    RegistryClient, RegistryEvents, HookStateKeys, MemoTypes,
    Defaults, EvernodeConstants
} = require('evernode-js-client');
const { Buffer } = require('buffer');
const { FirestoreManager } = require('./lib/firestore-manager');

const BETA_STATE_INDEX = ""; // This constant will be populated when beta firebase project is created.
const MIN_XRP = "1";
const INIT_MEMO_TYPE = "evnInitialize"; // This is kept only here as a constant, since we don't want to expose this event to public.
const INIT_MEMO_FORMAT = "hex";

const RIPPLED_URL = process.env.RIPPLED_URL || "wss://hooks-testnet-v2.xrpl-labs.com";
const MODE = process.env.MODE || 'dev';
const ACTION = process.env.ACTION || 'run';

const DATA_DIR = process.env.DATA_DIR || __dirname;

const CONFIG_FILE = 'accounts.json';
const HOOK_DATA_DIR = DATA_DIR + '/data';
const FIREBASE_SEC_KEY_PATH = DATA_DIR + '/service-acc/firebase-sa-key.json';

const HOOK_INITIALIZER = {
    address: 'rMv668j9M6x2ww4HNEF4AhB8ju77oSxFJD',
    secret: 'sn6TNZivVQY9KxXrLy8XdH9oXk3aG'
}

const NFT_WAIT_TIMEOUT = 80;
const MAX_BATCH_SIZE = 500;
const PROCESS_INTERVAL = 10000; // in milliseconds.
let PROCESS_LOCK = false;

const AFFECTED_HOOK_STATE_MAP = {
    INIT: [
        // Configs
        HookStateKeys.EVR_ISSUER_ADDR,
        HookStateKeys.FOUNDATION_ADDR,
        HookStateKeys.MOMENT_SIZE,
        HookStateKeys.MINT_LIMIT,
        HookStateKeys.FIXED_REG_FEE,
        HookStateKeys.HOST_HEARTBEAT_FREQ,
        HookStateKeys.PURCHASER_TARGET_PRICE,
        HookStateKeys.LEASE_ACQUIRE_WINDOW,

        // Singleton
        HookStateKeys.HOST_COUNT,
        HookStateKeys.MOMENT_BASE_IDX,
        HookStateKeys.HOST_REG_FEE,
        HookStateKeys.MAX_REG
    ],
    HEARTBEAT: [
        // NOTE: Repetetative State keys
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    HOST_REG: [
        HookStateKeys.HOST_COUNT,
        HookStateKeys.FIXED_REG_FEE,
        HookStateKeys.MAX_REG,

        // NOTE: Repetetative State keys
        // HookStateKeys.PREFIX_HOST_TOKENID,
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    HOST_DEREG: [
        HookStateKeys.HOST_COUNT

        // NOTE: Repetetative State keys
        // HookStateKeys.PREFIX_HOST_TOKENID,
    ],
    HOST_UPDATE_REG: [
        // NOTE: Repetetative State keys
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    HOST_POST_DEREG: [
        // NOTE: Repetetative State keys
        // HookStateKeys.PREFIX_HOST_ADDR
        // HookStateKeys.PREFIX_HOST_TOKENID
    ]
}

/**
 * Registry Index Manager listens to the transactions on the registry account and update Firebase Index accordingly.
 */
class IndexManager {
    #firestoreManager = null;
    #xrplApi = null;
    #xrplAcc = null;
    #registryClient = null;
    #pendingTransactions = null;

    constructor(rippledServer, registryAddress, stateIndexId = null) {
        this.#xrplApi = new XrplApi(rippledServer);
        Defaults.set({
            registryAddress: registryAddress,
            rippledServer: rippledServer,
            xrplApi: this.#xrplApi
        })
        this.#xrplAcc = new XrplAccount(registryAddress);
        this.#firestoreManager = new FirestoreManager(stateIndexId ? { stateIndexId: stateIndexId } : {});
        this.#registryClient = new RegistryClient(registryAddress);
        this.#pendingTransactions = [];
    }

    async init(firebaseSecKeyPath) {
        try {
            await this.#xrplApi.connect();
            await this.#connectRegistry();
            await this.#registryClient.subscribe();
            await this.#firestoreManager.authorize(firebaseSecKeyPath);
            this.config = await this.#firestoreManager.getConfigs();
            if (!this.config || !this.config.length) {
                const states = await this.#registryClient.getHookStates();
                if (!states || !states.length)
                    throw { code: 'NO_STATE_KEY' };

                await Promise.all(states.map(async state => {
                    const decoded = StateHelpers.decodeStateData(Buffer.from(state.key, 'hex'), Buffer.from(state.data, 'hex'));
                    if (decoded.type == StateHelpers.StateTypes.SIGLETON || decoded.type == StateHelpers.StateTypes.CONFIGURATION)
                        await this.#firestoreManager.setConfig(decoded);
                }));
            }
        } catch (e) {
            if (e.code === "NO_STATE_KEY") {
                console.log(`Waiting for hook initialize transaction (${this.#xrplAcc.address})...`);
                await new Promise(async (resolve) => {
                    await this.#registryClient.subscribe()
                    await this.#registryClient.on(RegistryEvents.RegistryInitialized, async (data) => {
                        await this.handleTransaction(data);
                        await this.#registryClient.connect();
                        resolve();
                    });
                });
            }
            else {
                throw e;
            }
        }

        if (ACTION === 'recover') {
            await this.#recover()
        }

        this.#registryClient.on(RegistryEvents.HostRegistered, data => { this.#pendingTransactions.push(data) });
        this.#registryClient.on(RegistryEvents.HostDeregistered, data => { this.#pendingTransactions.push(data) });
        this.#registryClient.on(RegistryEvents.HostRegUpdated, data => { this.#pendingTransactions.push(data) });
        this.#registryClient.on(RegistryEvents.Heartbeat, data => { this.#pendingTransactions.push(data) });
        this.#registryClient.on(RegistryEvents.HostPostDeregistered, data => { this.#pendingTransactions.push(data) });

        console.log(`Listening to registry address (${this.#xrplAcc.address})...`);

        const idxManager = this;
        // Interval based schedule to process the pending transactions.
        let timerId = setTimeout(function tick() {
            if (!PROCESS_LOCK) {
                idxManager.#handleTransactions();
            }
            timerId = setTimeout(tick, PROCESS_INTERVAL);
        }, PROCESS_INTERVAL);

    }

    // Connect the registry and trying to reconnect in the event of account not found error.
    // Account not found error can be because of a network reset. (Dev and test nets)
    async #connectRegistry() {
        let attempts = 0;
        // eslint-disable-next-line no-constant-condition
        while (true) {
            try {
                attempts++;
                const ret = await this.#registryClient.connect();
                if (ret)
                    break;
            } catch (error) {
                if (error?.data?.error === 'actNotFound') {
                    let delaySec;
                    // The maximum delay will be 5 minutes.
                    if (attempts > 150) {
                        delaySec = 300;
                    } else {
                        delaySec = 2 * attempts;
                    }
                    console.log(`Network reset detected. Attempt ${attempts} failed. Retrying in ${delaySec}s...`);
                    await new Promise(resolve => setTimeout(resolve, delaySec * 1000));
                } else
                    throw error;
            }
        }
    }

    // To update index, if the the service is down for a long period.
    async #recover() {
        const statesInIndex = (await this.#firestoreManager.getHosts()).map(h => h.id);
        await this.#persisit(statesInIndex, true);
    }

    // To process pending transactions. (Takes a batch and precess.)
    async #handleTransactions() {
        PROCESS_LOCK = true;

        // Top N (MAX_BATCH_SIZE) batch of pending transactions.
        const processingTrxns = this.#pendingTransactions.slice(0, MAX_BATCH_SIZE);
        try {
            if (processingTrxns.length == 0)
                throw "No transactions were found to process."

            console.log(`|Tot. trx: ${processingTrxns.length}|Batch process started.`);
            for (let item of processingTrxns) {
                await this.#processTransaction(item);
            }
            // Remove the processed transactions.
            this.#pendingTransactions.splice(0, processingTrxns.length);
            console.log(`|Tot. trx: ${processingTrxns.length}|Batch process completed.`);
        }
        catch (e) {
            console.error(e);
        }
        finally {
            PROCESS_LOCK = false;
        }
    }

    // Process the transaction.
    async #processTransaction(data) {
        const trx = data.transaction;
        let affectedStates = [];
        let stateKeyHostAddrId = null;
        let stateKeyTokenId = null;

        const hostTrxs = [MemoTypes.HOST_REG, MemoTypes.HOST_DEREG, MemoTypes.HOST_UPDATE_INFO, MemoTypes.HEARTBEAT, MemoTypes.HOST_POST_DEREG];

        const memoType = trx.Memos[0].type;
        console.log(`|${trx.Account}|${memoType}|Fetched a transaction from the batch`);

        // HOST_ADDR State Key
        if (hostTrxs.includes(memoType))
            stateKeyHostAddrId = StateHelpers.generateHostAddrStateKey(trx.Account);

        switch (memoType) {
            case MemoTypes.REGISTRY_INIT:
                affectedStates = AFFECTED_HOOK_STATE_MAP.INIT;
                break;
            case MemoTypes.HOST_REG: {
                affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_REG;
                affectedStates.push(stateKeyHostAddrId.toString('hex').toUpperCase());

                const uri = `${EvernodeConstants.NFT_PREFIX_HEX}${trx.hash}`;
                let regNft = null;
                const hostXrplAcc = new XrplAccount(trx.Account);
                let attempts = 0;
                while (attempts < NFT_WAIT_TIMEOUT) {
                    // Check in Registry.
                    regNft = (await this.#xrplAcc.getNfts()).find(n => (n.URI === uri));
                    if (!regNft) {
                        // Check in Host.
                        regNft = (await hostXrplAcc.getNfts()).find(n => (n.URI === uri));
                    }

                    if (regNft) {
                        break;
                    }

                    await new Promise(r => setTimeout(r, 1000));
                    attempts++;
                }

                if (!regNft) {
                    console.log(`|${trx.Account}|${memoType}|No Reg. NFT was found within the timeout.`);
                    break;
                }

                stateKeyTokenId = StateHelpers.generateTokenIdStateKey(regNft.NFTokenID);
                affectedStates.push(stateKeyTokenId);
                break;
            }
            case MemoTypes.HOST_DEREG:
                affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_DEREG;
                affectedStates.push(stateKeyHostAddrId);
                break;
            case MemoTypes.HOST_UPDATE_INFO:
                affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_UPDATE_REG;
                affectedStates.push(stateKeyHostAddrId);
                break;
            case MemoTypes.HEARTBEAT:
                affectedStates = AFFECTED_HOOK_STATE_MAP.HEARTBEAT;
                affectedStates.push(stateKeyHostAddrId);
                break;
            case MemoTypes.HOST_POST_DEREG: {
                affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_POST_DEREG;
                affectedStates.push(stateKeyHostAddrId);

                const nftTokenId = trx.Memos[0].data;
                stateKeyTokenId = StateHelpers.generateTokenIdStateKey(nftTokenId);
                affectedStates.push(stateKeyTokenId);
                break;
            }
        }

        await this.#persisit(affectedStates);
        console.log(`|${trx.Account}|${memoType}|Completed transaction processing`);
    }

    async #persisit(affectedStates, doRecover = false) {

        const hookStates = await this.#registryClient.getHookStates();
        const updatedStates = (doRecover) ? hookStates : (hookStates).filter(s => affectedStates.includes(s.key));
        if (!updatedStates) {
            console.log("No state entries were found for this hook account");
            return;
        }

        // To find removed states.
        const updatedStateKeys = updatedStates.map(s => s.key);
        const removedStates = (affectedStates.filter(s => !(updatedStateKeys.includes(s)))).map(st => { return { key: st } });

        let indexUpdates = {
            set: {
                hosts: {},
                configs: {}
            },
            delete: {
                hosts: {},
                configs: {}
            }
        };

        const updateIndexSet = (key, value) => {
            const decoded = StateHelpers.decodeStateData(key, value);
            // If the object already exists we override it.
            // We combine host address and token objects.
            if (decoded.type == StateHelpers.StateTypes.HOST_ADDR || decoded.type == StateHelpers.StateTypes.TOKEN_ID) {
                // If this is a token id update we replace the key with address key,
                // So the existing host address state will get updated.
                if (decoded.type == StateHelpers.StateTypes.TOKEN_ID) {
                    decoded.key = decoded.addressKey;
                    delete decoded.addressKey;
                }

                delete decoded.type;
                indexUpdates.set.hosts[decoded.key] = {
                    ...(indexUpdates.set.hosts[decoded.key] ? indexUpdates.set.hosts[decoded.key] : {}),
                    ...decoded
                }
            }
            else if (decoded.type == StateHelpers.StateTypes.SIGLETON || decoded.type == StateHelpers.StateTypes.CONFIGURATION)
                indexUpdates.set.configs[decoded.key] = decoded;
        }

        const deleteIndexSet = (key) => {
            const decoded = StateHelpers.decodeStateKey(key);

            if (decoded.type == StateHelpers.StateTypes.HOST_ADDR)
                indexUpdates.delete.hosts[decoded.key] = true;

            else if (decoded.type == StateHelpers.StateTypes.SIGLETON || decoded.type == StateHelpers.StateTypes.CONFIGURATION)
                indexUpdates.delete.configs[decoded.key] = true;

        }

        const persistIndex = async () => {
            // Persist the data to the firestore index.
            // Since we only have one list object per entry,
            // There's only one insert, update or delete operation per document.
            // So we can promise.all all the inserts,updates and deletes.
            await Promise.all([
                ...Object.values(indexUpdates.set.hosts).map(async obj => {
                    await this.#firestoreManager.setHost(obj);
                }),
                ...Object.values(indexUpdates.set.configs).map(async obj => {
                    await this.#firestoreManager.setConfig(obj);
                }),
                ...Object.keys(indexUpdates.delete.hosts).map(async key => {
                    await this.#firestoreManager.deleteHost(key);
                }),
                ...Object.keys(indexUpdates.delete.configs).map(async key => {
                    await this.#firestoreManager.deleteConfig(key);
                })
            ]);
        }

        // Prepare for inset and update.
        updatedStates.forEach(state => {
            const keyBuf = Buffer.from(state.key, 'hex');
            const valueBuf = Buffer.from(state.data, 'hex');
            updateIndexSet(keyBuf, valueBuf);

        });

        // Prepare for delete.
        removedStates.forEach(state => {
            const keyBuf = Buffer.from(state.key, 'hex');
            deleteIndexSet(keyBuf);
        });

        await persistIndex();
    }
}

async function initRegistryConfigs(config, configPath, rippledServer) {
    // Get issuer and foundation cold wallet account ids.
    let memoData = Buffer.allocUnsafe(40);
    codec.decodeAccountID(config.issuer.address).copy(memoData);
    codec.decodeAccountID(config.foundationColdWallet.address).copy(memoData, 20);

    const xrplApi = new XrplApi(rippledServer);
    await xrplApi.connect();
    const initAccount = new XrplAccount(HOOK_INITIALIZER.address, HOOK_INITIALIZER.secret, { xrplApi: xrplApi });
    const res = await initAccount.makePayment(config.registry.address, MIN_XRP, 'XRP', null,
        [{ type: INIT_MEMO_TYPE, format: INIT_MEMO_FORMAT, data: memoData.toString('hex') }]);

    if (res.code === 'tesSUCCESS') {
        // Listen for the transaction with tx hash.
        // Update the config initialized flag and write the config.
        config.initialized = true;
        fs.writeFileSync(configPath, JSON.stringify(config, null, 4));
        return res;
    }
    else {
        throw res;
    }
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

    // Firestore Service Account key
    if (!fs.existsSync(FIREBASE_SEC_KEY_PATH)) {
        console.error(`${FIREBASE_SEC_KEY_PATH} not found, place the json file in the location.`);
        return;
    }

    // Send the config init transaction to the registry account.
    if (!config.initialized) {
        console.log('Sending registry contract initialization transation.');
        const res = await initRegistryConfigs(config, configPath, RIPPLED_URL).catch(e => {
            throw `Registry contract initialization transaction failed with ${e}.`;
        });
        if (res)
            console.log('Registry contract initialization success.')
    }

    // Start the Index Manager.
    const indexManager = new IndexManager(RIPPLED_URL, config.registry.address, MODE === 'beta' ? BETA_STATE_INDEX : null);
    await indexManager.init(FIREBASE_SEC_KEY_PATH);
}

main().catch(e => {
    console.error(e);
    process.exit(1);
});
