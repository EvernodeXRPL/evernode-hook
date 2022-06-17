const fs = require('fs');
const process = require('process');
const path = require('path');
const codec = require('ripple-address-codec');
const evernode = require('evernode-js-client');
const { XrplApi, StateHelpers, RegistryClient, Defaults, RegistryEvents, } = require('evernode-js-client');
const { Buffer } = require('buffer');
const { FirestoreManager } = require('./lib/firestore-manager');

const BETA_STATE_INDEX = ""; // This constant will be populated when beta firebase project is created.

const RIPPLED_URL = process.env.RIPPLED_URL || "wss://hooks-testnet-v2.xrpl-labs.com";
const MODE = process.env.MODE || 'dev';

const DATA_DIR = process.env.DATA_DIR || __dirname;
const BIN_DIR = process.env.BIN_DIR || path.resolve(__dirname, 'dist');

const CONFIG_FILE = 'config.json';
const HOOK_DATA_DIR = DATA_DIR + '/data';
const FIREBASE_SEC_KEY_PATH = DATA_DIR + '/service-acc/firebase-sa-key.json';

const AFFECTED_HOOK_STATE_MAP = {
    "INIT": [
        // Configs
        evernode.HookStateKeys.EVR_ISSUER_ADDR,
        evernode.HookStateKeys.FOUNDATION_ADDR,
        evernode.HookStateKeys.MOMENT_SIZE,
        evernode.HookStateKeys.MINT_LIMIT,
        evernode.HookStateKeys.FIXED_REG_FEE,
        evernode.HookStateKeys.HOST_HEARTBEAT_FREQ,
        evernode.HookStateKeys.PURCHASER_TARGET_PRICE,
        evernode.HookStateKeys.LEASE_ACQUIRE_WINDOW,

        // Singleton
        evernode.HookStateKeys.HOST_COUNT,
        evernode.HookStateKeys.MOMENT_BASE_IDX,
        evernode.HookStateKeys.HOST_REG_FEE,
        evernode.HookStateKeys.MAX_REG
    ],
    "HEARTBEAT": [
        // NOTE: Repetetative State keys
        // evernode.HookStateKeys.PREFIX_HOST_ADDR
    ],
    "HOST_REG": [
        evernode.HookStateKeys.HOST_COUNT,
        evernode.HookStateKeys.FIXED_REG_FEE,
        evernode.HookStateKeys.MAX_REG,

        // NOTE: Repetetative State keys
        // evernode.HookStateKeys.PREFIX_HOST_TOKENID,
        // evernode.HookStateKeys.PREFIX_HOST_ADDR
    ],
    "HOST_DEREG": [
        evernode.HookStateKeys.HOST_COUNT

        // NOTE: Repetetative State keys
        // evernode.HookStateKeys.PREFIX_HOST_TOKENID,
    ],
    "HOST_UPDATE_REG": [
        // NOTE: Repetetative State keys
        // evernode.HookStateKeys.PREFIX_HOST_ADDR
    ],
    "HOST_POST_DEREG": [
        // NOTE: Repetetative State keys
        // evernode.HookStateKeys.PREFIX_HOST_ADDR
        // evernode.HookStateKeys.PREFIX_HOST_TOKENID
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

    constructor(rippledServer, registryAddress, registrySecret, stateIndexId = null) {
        this.#xrplApi = new XrplApi(rippledServer);
        Defaults.set({
            registryAddress: registryAddress,
            rippledServer: rippledServer,
            xrplApi: this.#xrplApi
        })
        this.#xrplAcc = new evernode.XrplAccount(registryAddress, registrySecret);
        this.#firestoreManager = new FirestoreManager(stateIndexId ? { stateIndexId: stateIndexId } : {});
        this.#registryClient = new RegistryClient(registryAddress, registrySecret);
    }

    async init(firebaseSecKeyPath) {
        try {
            await this.#xrplApi.connect();
            await this.#firestoreManager.authorize(firebaseSecKeyPath);
            await this.#registryClient.subscribe()
            await this.#registryClient.connect();
            this.config = await this.#firestoreManager.getConfigs();
            if (!this.config)
                await this.#firestoreManager.setConfig(this.#registryClient.config);
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
                console.error(e.error);
                return;
            }
        }

        if (MODE === 'recover') {
            await this.#recover()
        }

        this.#registryClient.on(RegistryEvents.HostRegistered, data => this.handleTransaction(data));
        this.#registryClient.on(RegistryEvents.HostDeregistered, data => this.handleTransaction(data));
        this.#registryClient.on(RegistryEvents.HostRegUpdated, data => this.handleTransaction(data));
        this.#registryClient.on(RegistryEvents.Heartbeat, data => this.handleTransaction(data));
        this.#registryClient.on(RegistryEvents.HostPostDeregistered, data => this.handleTransaction(data));

        console.log(`Listening to registry address (${this.#xrplAcc.address})...`);

    }

    // To update index, if the the service is down for a long period.

    async #recover() {
        const statesInIndex = (await this.#firestoreManager.getHosts()).map(h => h.id);
        await this.#persisit(statesInIndex, true);
    }

    // To update index on the go.
    async handleTransaction(data) {

        const trx = data.transaction;
        let affectedStates = [];
        let stateKeyTokenIdArr = [];
        let stateKeyHostAddrId = null;
        let stateKeyTokenId = null;

        const hostTrxs = [evernode.MemoTypes.HOST_REG, evernode.MemoTypes.HOST_DEREG, evernode.MemoTypes.HOST_UPDATE_INFO, evernode.MemoTypes.HEARTBEAT, evernode.MemoTypes.HOST_POST_DEREG];

        const memoType = trx.Memos[0].type;
        console.log(`Triggered a Transaction - ${trx.TransactionType} -> ${memoType}`);

        // HOST_ADDR State Key
        if (hostTrxs.includes(memoType)) {

            let buf = Buffer.allocUnsafe(9);
            buf.writeUInt8(3);
            for (let i = 1; i < 9; i++) {
                buf.writeUInt8(0, i);
            }
            const addrBuf = Buffer.from(codec.decodeAccountID(trx.Account), "hex");
            stateKeyHostAddrId = Buffer.concat([Buffer.from("EVR", "utf-8"), buf, addrBuf]);
        }

        switch (memoType) {
            case evernode.MemoTypes.REGISTRY_INIT:
                affectedStates = AFFECTED_HOOK_STATE_MAP['INIT'];
                break;
            case evernode.MemoTypes.HOST_REG:
                affectedStates = AFFECTED_HOOK_STATE_MAP['HOST_REG'];
                affectedStates.push(stateKeyHostAddrId.toString('hex').toUpperCase());
                const uri = `${evernode.EvernodeConstants.NFT_PREFsIX_HEX}${trx.hash}`;

                // 5 seconds wait untill the NFT is available.
                await new Promise(r => setTimeout(r, 5000));
                let regNft = (await this.#xrplAcc.getNfts()).find(n => (n.URI === uri));

                if (!regNft) {
                    const hostXrplAcc = new evernode.XrplAccount(trx.Account);
                    const nfts = await hostXrplAcc.getNfts();
                    regNft = (nfts).find(n => (n.URI === uri));
                    if (!regNft) {
                        // Exit if NFT was not found.
                        break;
                    }
                }

                stateKeyTokenId = await this.#generateTokenIdStateKey(regNft.NFTokenID);
                affectedStates.push(stateKeyTokenId);
                break;
            case evernode.MemoTypes.HOST_DEREG:
                affectedStates = AFFECTED_HOOK_STATE_MAP['HOST_DEREG'];
                affectedStates.push(stateKeyHostAddrId.toString('hex').toUpperCase());
                break;
            case evernode.MemoTypes.HOST_UPDATE_INFO:
                affectedStates = AFFECTED_HOOK_STATE_MAP['HOST_UPDATE_REG'];
                affectedStates.push(stateKeyHostAddrId.toString('hex').toUpperCase());
                break;
            case evernode.MemoTypes.HEARTBEAT:
                affectedStates = AFFECTED_HOOK_STATE_MAP['HEARTBEAT'];
                affectedStates.push(stateKeyHostAddrId.toString('hex').toUpperCase());
                break;
            case evernode.MemoTypes.HOST_POST_DEREG:
                affectedStates = AFFECTED_HOOK_STATE_MAP['HOST_POST_DEREG'];
                affectedStates.push(stateKeyHostAddrId.toString('hex').toUpperCase());

                const nftTokenId = trx.Memos[0].data;
                stateKeyTokenId = await this.#generateTokenIdStateKey(nftTokenId);
                affectedStates.push(stateKeyTokenId);
                break;
        }

        await this.#persisit(affectedStates);
    }

    async #generateTokenIdStateKey(nfTokenId) {
        // TOKEN_ID State Keys
        let buf = Buffer.allocUnsafe(1);
        buf.writeUInt8(2);
        const nfTokenIdBuf = Buffer.from(nfTokenId, "hex");
        const stateKeyBuf = (Buffer.concat([Buffer.from("EVR", "utf-8"), buf, nfTokenIdBuf.slice(4, 32)]));
        return stateKeyBuf.toString('hex').toUpperCase();
    }

    async #persisit(affectedStates, doRecover = false) {

        const hookStates = await this.#registryClient.getHookStates();
        const rawStates = (doRecover) ? hookStates : (hookStates).filter(s => affectedStates.includes(s.key));
        if (!rawStates) {
            console.log("No state entries were found for this hook account");
            return;
        }

        // To find removed states.
        const rawStatesKeys = rawStates.map(s => s.key);
        const removedStates = (affectedStates.filter(s => !(rawStatesKeys.includes(s)))).map(st => { return { key: st } });

        let states = rawStates.map(s => { return { key: s.key, value: s.data } });
        states = states.concat(removedStates);

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
            // We are sure these will get called sequentially.
            // If the deleted object exists in set arrays, delete the object there since it's get anyway deleted.
            if (decoded.type == StateHelpers.StateTypes.HOST_ADDR) {
                delete indexUpdates.set.hosts[decoded.key];
                indexUpdates.delete.hosts[decoded.key] = true;
            }
            else if (decoded.type == StateHelpers.StateTypes.SIGLETON || decoded.type == StateHelpers.StateTypes.CONFIGURATION) {
                delete indexUpdates.set.configs[decoded.key];
                indexUpdates.delete.configs[decoded.key] = true;
            }
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

        for (const state of states) {
            const keyBuf = Buffer.from(state.key, 'hex');
            if (state?.value) {
                const valueBuf = Buffer.from(state.value, 'hex');
                updateIndexSet(keyBuf, valueBuf);
            }
            else {
                deleteIndexSet(keyBuf);
            }
        }

        await persistIndex();
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

    // Start the Index Manager.
    const indexManager = new IndexManager(RIPPLED_URL, config.registry.address, config.registry.secret, MODE === 'beta' ? BETA_STATE_INDEX : null);
    await indexManager.init(FIREBASE_SEC_KEY_PATH);

}

main().catch(console.error);
