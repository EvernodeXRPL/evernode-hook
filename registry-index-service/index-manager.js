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

const ACCOUNT_CONFIG_FILE = 'accounts.json';
const CONFIG_FILE = 'index-manager.json';
const HOOK_DATA_DIR = DATA_DIR + '/data';
const FIREBASE_SEC_KEY_PATH = DATA_DIR + '/service-acc/firebase-sa-key.json';

const NFT_WAIT_TIMEOUT = 80;
const MAX_BATCH_SIZE = 500;
const PROCESS_INTERVAL = 20000; // in milliseconds.
let PROCESS_LOCK = false;

const AFFECTED_HOOK_STATE_MAP = {
    INIT: [
        // Configs

        { operation: 'INSERT', key: HookStateKeys.EVR_ISSUER_ADDR },
        { operation: 'INSERT', key: HookStateKeys.FOUNDATION_ADDR },
        { operation: 'INSERT', key: HookStateKeys.MOMENT_SIZE },
        { operation: 'INSERT', key: HookStateKeys.MINT_LIMIT },
        { operation: 'INSERT', key: HookStateKeys.FIXED_REG_FEE },
        { operation: 'INSERT', key: HookStateKeys.HOST_HEARTBEAT_FREQ },
        { operation: 'INSERT', key: HookStateKeys.PURCHASER_TARGET_PRICE },
        { operation: 'INSERT', key: HookStateKeys.LEASE_ACQUIRE_WINDOW },
        { operation: 'INSERT', key: HookStateKeys.MAX_TOLERABLE_DOWNTIME },
        { operation: 'INSERT', key: HookStateKeys.REWARD_CONFIGURATION },
        { operation: 'INSERT', key: HookStateKeys.MOMENT_TRANSIT_INFO },

        // Singleton
        { operation: 'INSERT', key: HookStateKeys.HOST_COUNT },
        { operation: 'INSERT', key: HookStateKeys.MOMENT_BASE_INFO },
        { operation: 'INSERT', key: HookStateKeys.HOST_REG_FEE },
        { operation: 'INSERT', key: HookStateKeys.MAX_REG },
        { operation: 'UPDATE', key: HookStateKeys.REWARD_INFO }
    ],
    HEARTBEAT: [
        { operation: 'UPDATE', key: HookStateKeys.REWARD_INFO }

        // NOTE: Repetetative State keys
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    HOST_REG: [
        { operation: 'UPDATE', key: HookStateKeys.HOST_COUNT },
        { operation: 'UPDATE', key: HookStateKeys.HOST_REG_FEE },
        { operation: 'UPDATE', key: HookStateKeys.MAX_REG }

        // NOTE: Repetetative State keys
        // HookStateKeys.PREFIX_HOST_TOKENID
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    HOST_DEREG: [
        { operation: 'UPDATE', key: HookStateKeys.HOST_COUNT },
        { operation: 'UPDATE', key: HookStateKeys.REWARD_INFO }
    ],
    HOST_UPDATE_REG: [
        // NOTE: Repetetative State keys
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    HOST_POST_DEREG: [
        // NOTE: Repetetative State keys
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    DEAD_HOST_PRUNE: [
        { operation: 'UPDATE', key: HookStateKeys.HOST_COUNT },
        { operation: 'UPDATE', key: HookStateKeys.REWARD_INFO }
        // NOTE: Repetetative State keys
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    HOST_TRANSFER: [
        // NOTE: Repetetative State keys
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    HOST_REBATE: [
        // NOTE: Repetetative State keys
        // HookStateKeys.PREFIX_HOST_ADDR
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
    #queuedStates = null;

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
        this.#queuedStates = [];
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
                        await this.#updateStatesKeyQueue(data);
                        await this.#registryClient.connect();
                        resolve();
                    });
                });
            }
            else {
                throw e;
            }
        }

        // This step helps to recover the missed transaction details.
        if (ACTION === 'recover') {
            await this.#recover()
        }

        this.#registryClient.on(RegistryEvents.RegistryInitialized, async (data) => { await this.#updateStatesKeyQueue(data) });
        this.#registryClient.on(RegistryEvents.HostRegistered, async (data) => { await this.#updateStatesKeyQueue(data) });
        this.#registryClient.on(RegistryEvents.HostDeregistered, async (data) => { await this.#updateStatesKeyQueue(data) });
        this.#registryClient.on(RegistryEvents.HostRegUpdated, async (data) => { await this.#updateStatesKeyQueue(data) });
        this.#registryClient.on(RegistryEvents.Heartbeat, async (data) => { await this.#updateStatesKeyQueue(data) });
        this.#registryClient.on(RegistryEvents.HostPostDeregistered, async (data) => { await this.#updateStatesKeyQueue(data) });
        this.#registryClient.on(RegistryEvents.DeadHostPrune, async (data) => { await this.#updateStatesKeyQueue(data) });
        this.#registryClient.on(RegistryEvents.HostRebate, async (data) => { await this.#updateStatesKeyQueue(data) });
        this.#registryClient.on(RegistryEvents.HostTransfer, async (data) => { await this.#updateStatesKeyQueue(data) });


        console.log(`Listening to registry address (${this.#xrplAcc.address})...`);

        // Interval based scheduler to process the pending transactions.
        const doUpdate = () => {
            if (!PROCESS_LOCK) {
                this.#processStateQueue();
            }
        }

        setTimeout(function tick() {
            doUpdate();
            setTimeout(tick, PROCESS_INTERVAL);
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

    // Update state queue according to the listened transaction event.
    async #updateStatesKeyQueue(data) {
        const affectedStates = await this.#getTransactionSummary(data);
        affectedStates.map(s => {
            const found = this.#queuedStates.some(qs => (qs.key === s.key) && (qs.operation === s.operation));
            if (!found)
                this.#queuedStates.push({ operation: s.operation, key: s.key });
        });
    }

    // To update index, if the the service is down for a considerable period.
    async #recover() {
        try {
            const hosts = await this.#registryClient.getAllHosts();
            const configs = await this.#registryClient.getAllConfigs();

            const itemsInIndex = configs.concat(hosts);
            const stateKeyList = itemsInIndex.map(item => item.id);

            await this.#recoveryIndexUpdate(stateKeyList);
        } catch (e) {
            console.error(e);
        }
    }

    // To process pending states in the queue. (Takes a batch and process.)
    async #processStateQueue() {
        PROCESS_LOCK = true;

        try {
            // Top N (MAX_BATCH_SIZE) batch of pending states.
            const processingStates = this.#queuedStates.slice(0, MAX_BATCH_SIZE);

            if (processingStates.length > 0) {
                console.log(`|Tot. states: ${processingStates.length}|Batch process started.`);
                await this.#updateIndexStates(processingStates);

                // Remove the processed states.
                this.#queuedStates.splice(0, processingStates.length);
                console.log(`|Tot. states: ${processingStates.length}|Batch process completed.`);
            }
        }
        catch (e) {
            console.error(e);
        }
        finally {
            PROCESS_LOCK = false;
        }
    }

    // To get the summary of the listened trasaction (offected states with the operation).
    async #getTransactionSummary(data) {
        const trx = data.transaction;
        let affectedStates = [];
        let stateKeyHostAddrId = null;
        let stateKeyTokenId = null;

        const hostTrxs = [MemoTypes.HOST_REG, MemoTypes.HOST_DEREG, MemoTypes.HOST_UPDATE_INFO, MemoTypes.HEARTBEAT, MemoTypes.HOST_POST_DEREG, MemoTypes.DEAD_HOST_PRUNE, MemoTypes.HOST_REBATE, MemoTypes.HOST_TRANSFER];

        const memoType = trx.Memos[0].type;
        console.log(`|${trx.Account}|${memoType}|Triggered a transaction.`);

        try {
            // HOST_ADDR State Key
            if (hostTrxs.includes(memoType))
                stateKeyHostAddrId = StateHelpers.generateHostAddrStateKey((memoType !== MemoTypes.DEAD_HOST_PRUNE) ? trx.Account : data.host);

            switch (memoType) {
                case MemoTypes.REGISTRY_INIT:
                    affectedStates = AFFECTED_HOOK_STATE_MAP.INIT.slice();
                    break;
                case MemoTypes.HOST_REG: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_REG.slice();
                    affectedStates.push({ operation: 'INSERT', key: stateKeyHostAddrId.toString('hex').toUpperCase() });

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
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyTokenId });
                    break;
                }
                case MemoTypes.HOST_DEREG:
                    affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_DEREG.slice();
                    break;
                case MemoTypes.HOST_UPDATE_INFO:
                    affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_UPDATE_REG.slice();
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyHostAddrId });
                    break;
                case MemoTypes.HEARTBEAT:
                    affectedStates = AFFECTED_HOOK_STATE_MAP.HEARTBEAT.slice();
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyHostAddrId });
                    break;
                case MemoTypes.HOST_POST_DEREG: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_POST_DEREG.slice();
                    affectedStates.push({ operation: 'DELETE', key: stateKeyHostAddrId });
                    break;
                }
                case MemoTypes.DEAD_HOST_PRUNE: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.DEAD_HOST_PRUNE.slice();
                    affectedStates.push({ operation: 'DELETE', key: stateKeyHostAddrId });
                    break;
                }
                case MemoTypes.HOST_REBATE: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_REBATE.slice();
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyHostAddrId });
                    break;
                }
                case MemoTypes.HOST_TRANSFER: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_TRANSFER.slice();
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyHostAddrId });
                    break;
                }
            }

            console.log(`|${trx.Account}|${memoType}|Completed fetching transaction data`);
        }
        catch (e) {
            console.error(e);
        }

        return affectedStates
    }

    // To update the Firestore index with a set of pending states in the queue.
    async #updateIndexStates(processingStates) {
        const inserts = [];
        const updates = [];
        const deletes = [];
        const hookStates = await this.#registryClient.getHookStates();

        processingStates.map(ps => {
            const found = hookStates.find(hs => (hs.key === ps.key));
            if (found)
                ps.data = found.data;
            if (ps.operation === 'INSERT' && ps.data)
                inserts.push(ps);
            else if (ps.operation === 'UPDATE' && ps.data)
                updates.push(ps);
            else
                deletes.push(ps);
        });

        await Promise.all(inserts.map(async ps => {
            const decoded = StateHelpers.decodeStateData(Buffer.from(ps.key, 'hex'), Buffer.from(ps.data, 'hex'));
            if (decoded.type == StateHelpers.StateTypes.SIGLETON || decoded.type == StateHelpers.StateTypes.CONFIGURATION)
                await this.#firestoreManager.setConfig(decoded);
            else {
                if (decoded.type == StateHelpers.StateTypes.TOKEN_ID) {
                    decoded.key = decoded.addressKey;
                    delete decoded.addressKey;
                }
                delete decoded.type;
                await this.#firestoreManager.setHost(decoded);
            }
        }));

        await Promise.all(updates.map(async ps => {
            const decoded = StateHelpers.decodeStateData(Buffer.from(ps.key, 'hex'), Buffer.from(ps.data, 'hex'));
            if (decoded.type == StateHelpers.StateTypes.SIGLETON || decoded.type == StateHelpers.StateTypes.CONFIGURATION)
                await this.#firestoreManager.setConfig(decoded, true);
            else {
                if (decoded.type == StateHelpers.StateTypes.TOKEN_ID) {
                    decoded.key = decoded.addressKey;
                    delete decoded.addressKey;
                }
                delete decoded.type;
                await this.#firestoreManager.setHost(decoded, true);
            }

        }));

        await Promise.all(deletes.map(async ps => {
            const decoded = StateHelpers.decodeStateKey(Buffer.from(ps.key, 'hex'));
            if (decoded.type == StateHelpers.StateTypes.SIGLETON || decoded.type == StateHelpers.StateTypes.CONFIGURATION)
                await this.#firestoreManager.deleteConfig(ps.key);
            else {
                if (decoded.type == StateHelpers.StateTypes.HOST_ADDR) {
                    await this.#firestoreManager.deleteHost(decoded.key);
                }
            }
        }));
    }

    // To update the Firestore index in a recovery situation.
    async #recoveryIndexUpdate(affectedStates) {

        const hookStates = await this.#registryClient.getHookStates();
        if (!hookStates) {
            throw "No state entries were found for this hook account";
        }

        // To find removed states.
        const hookStateKeys = hookStates.map(s => s.key);
        const removedStates = (affectedStates.filter(s => !(hookStateKeys.includes(s)))).map(st => { return { key: st } });

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
        hookStates.forEach(state => {
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

async function initRegistryConfigs(initializerInfo, config, accountConfigPath, rippledServer) {
    // Get issuer and foundation cold wallet account ids.
    let memoData = Buffer.allocUnsafe(40);
    codec.decodeAccountID(config.issuer.address).copy(memoData);
    codec.decodeAccountID(config.foundationColdWallet.address).copy(memoData, 20);

    const xrplApi = new XrplApi(rippledServer);
    await xrplApi.connect();
    const initAccount = new XrplAccount(initializerInfo.address, initializerInfo.secret, { xrplApi: xrplApi });
    const res = await initAccount.makePayment(config.registry.address, MIN_XRP, 'XRP', null,
        [{ type: INIT_MEMO_TYPE, format: INIT_MEMO_FORMAT, data: memoData.toString('hex') }]);

    if (res.code === 'tesSUCCESS') {
        // Listen for the transaction with tx hash.
        // Update the config initialized flag and write the config.
        config.initialized = true;
        fs.writeFileSync(accountConfigPath, JSON.stringify(config, null, 4));
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
    const configPath = path.resolve(DATA_DIR, CONFIG_FILE);
    const accountConfigPath = path.resolve(HOOK_DATA_DIR, registryAddress, ACCOUNT_CONFIG_FILE);

    // If configs doesn't exist, skip the execution.
    if (!fs.existsSync(configPath)) {
        console.error(`${configPath} not found.`);
        return;
    }
    else if (!fs.existsSync(accountConfigPath)) {
        console.error(`${accountConfigPath} not found, run the account setup tool and generate the accounts.`);
        return;
    }
    const config = JSON.parse(fs.readFileSync(configPath).toString());
    const accountConfig = JSON.parse(fs.readFileSync(accountConfigPath).toString());

    // If required files and config data doesn't exist, skip the execution.
    if (!config.hookInitializer || !config.hookInitializer.address || !config.hookInitializer.secret) {
        console.error('Hook initializer account info not found.');
        return;
    }
    else if (!accountConfig.registry || !accountConfig.registry.address || !accountConfig.registry.secret) {
        console.error('Registry account info not found, run the account setup tool and generate the accounts.');
        return;
    }

    // Firestore Service Account key
    if (!fs.existsSync(FIREBASE_SEC_KEY_PATH)) {
        console.error(`${FIREBASE_SEC_KEY_PATH} not found, place the json file in the location.`);
        return;
    }

    // Send the accountConfig init transaction to the registry account.
    if (!accountConfig.initialized) {
        console.log('Sending registry contract initialization transation.');
        const res = await initRegistryConfigs(config.hookInitializer, accountConfig, accountConfigPath, RIPPLED_URL).catch(e => {
            throw `Registry contract initialization transaction failed with ${e}.`;
        });
        if (res)
            console.log('Registry contract initialization success.')
    }

    // Start the Index Manager.
    const indexManager = new IndexManager(RIPPLED_URL, accountConfig.registry.address, MODE === 'beta' ? BETA_STATE_INDEX : null);
    await indexManager.init(FIREBASE_SEC_KEY_PATH);
}

main().catch(e => {
    console.error(e);
    process.exit(1);
});
