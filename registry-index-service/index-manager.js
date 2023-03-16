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
    XrplApi, XrplAccount, StateHelpers, TransactionHelper,
    GovernorEvents, HeartbeatEvents, RegistryEvents, HookClientFactory, HookTypes, HookStateKeys,
    Defaults, EvernodeConstants
} = require('evernode-js-client');
const { Buffer } = require('buffer');
const { FirestoreManager } = require('./lib/firestore-manager');

const BETA_STATE_INDEX = ""; // This constant will be populated when beta firebase project is created.
const MIN_XRP = "1";
const INIT_MEMO_TYPE = "evnInitialize"; // This is kept only here as a constant, since we don't want to expose this event to public.
const INIT_MEMO_FORMAT = "hex";

const RIPPLED_URL = process.env.RIPPLED_URL || "wss://hooks-testnet-v3.xrpl-labs.com";
const NETWORK_ID = process.env.NETWORK_ID || 21338;
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
    COMMON: [
        { operation: 'UPDATE', key: HookStateKeys.HOST_COUNT },
        { operation: 'UPDATE', key: HookStateKeys.MOMENT_BASE_INFO },
        { operation: 'UPDATE', key: HookStateKeys.HOST_REG_FEE },
        { operation: 'UPDATE', key: HookStateKeys.MAX_REG },
        { operation: 'UPDATE', key: HookStateKeys.REWARD_INFO }
    ],
    INIT: [
        // Configs

        { operation: 'INSERT', key: HookStateKeys.EVR_ISSUER_ADDR },
        { operation: 'INSERT', key: HookStateKeys.FOUNDATION_ADDR },
        { operation: 'INSERT', key: HookStateKeys.MOMENT_SIZE },
        { operation: 'INSERT', key: HookStateKeys.MINT_LIMIT },
        { operation: 'INSERT', key: HookStateKeys.FIXED_REG_FEE },
        { operation: 'INSERT', key: HookStateKeys.HOST_HEARTBEAT_FREQ },
        { operation: 'INSERT', key: HookStateKeys.LEASE_ACQUIRE_WINDOW },
        { operation: 'INSERT', key: HookStateKeys.MAX_TOLERABLE_DOWNTIME },
        { operation: 'INSERT', key: HookStateKeys.REWARD_CONFIGURATION },
        { operation: 'INSERT', key: HookStateKeys.MOMENT_TRANSIT_INFO },
        { operation: 'INSERT', key: HookStateKeys.MAX_TRX_EMISSION_FEE },
        { operation: 'INSERT', key: HookStateKeys.REGISTRY_ADDR },
        { operation: 'INSERT', key: HookStateKeys.HEARTBEAT_ADDR },
        { operation: 'INSERT', key: HookStateKeys.GOVERNANCE_CONFIGURATION },

        // Singleton
        { operation: 'INSERT', key: HookStateKeys.HOST_COUNT },
        { operation: 'INSERT', key: HookStateKeys.MOMENT_BASE_INFO },
        { operation: 'INSERT', key: HookStateKeys.HOST_REG_FEE },
        { operation: 'INSERT', key: HookStateKeys.MAX_REG },
        { operation: 'INSERT', key: HookStateKeys.REWARD_INFO },
        { operation: 'INSERT', key: HookStateKeys.GOVERNANCE_INFO }
    ],
    CANDIDATE_PROPOSE: [
        { operation: 'UPDATE', key: HookStateKeys.GOVERNANCE_INFO }

        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_CANDIDATE_OWNER
        // HookStateKeys.PREFIX_CANDIDATE_ID
    ],
    CANDIDATE_VOTE: [
        { operation: 'UPDATE', key: HookStateKeys.GOVERNANCE_INFO }

        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_CANDIDATE_ID
    ],
    CANDIDATE_WITHDRAW: [
        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_CANDIDATE_OWNER
        // HookStateKeys.PREFIX_CANDIDATE_ID
    ],
    DUD_HOST_REMOVE: [
        { operation: 'UPDATE', key: HookStateKeys.HOST_COUNT },
        { operation: 'UPDATE', key: HookStateKeys.REWARD_INFO }

        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_CANDIDATE_ID
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    DUD_HOST_STATUS_CHANGE: [
        { operation: 'UPDATE', key: HookStateKeys.REWARD_INFO }

        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_CANDIDATE_ID
    ],
    FALLBACK_PILOTED: [
        { operation: 'UPDATE', key: HookStateKeys.GOVERNANCE_INFO },

        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_CANDIDATE_ID
    ],
    CANDIDATE_ELECT: [
        { operation: 'UPDATE', key: HookStateKeys.GOVERNANCE_INFO },
        { operation: 'UPDATE', key: HookStateKeys.REWARD_INFO }

        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_CANDIDATE_ID
    ],
    CHILD_HOOK_UPDATE: [
        { operation: 'UPDATE', key: HookStateKeys.GOVERNANCE_INFO }

        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_CANDIDATE_ID
    ],
    CHANGE_GOVERNANCE_MODE: [
        { operation: 'UPDATE', key: HookStateKeys.GOVERNANCE_INFO }

        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_CANDIDATE_ID
    ],
    DUD_HOST_REPORT: [
        { operation: 'UPDATE', key: HookStateKeys.GOVERNANCE_INFO }

        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_CANDIDATE_ID
    ],
    HEARTBEAT: [
        { operation: 'UPDATE', key: HookStateKeys.REWARD_INFO },
        { operation: 'UPDATE', key: HookStateKeys.GOVERNANCE_INFO }

        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_HOST_ADDR
        // HookStateKeys.PREFIX_CANDIDATE_ID
    ],
    HOST_REG: [
        { operation: 'UPDATE', key: HookStateKeys.HOST_COUNT },
        { operation: 'UPDATE', key: HookStateKeys.HOST_REG_FEE },
        { operation: 'UPDATE', key: HookStateKeys.MAX_REG }

        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_HOST_TOKENID
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    HOST_DEREG: [
        { operation: 'UPDATE', key: HookStateKeys.HOST_COUNT },
        { operation: 'UPDATE', key: HookStateKeys.REWARD_INFO }

        // NOTE: Repetetative State keys
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    HOST_UPDATE_REG: [
        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_HOST_ADDR
        // HookStateKeys.PREFIX_HOST_TOKENID
    ],
    DEAD_HOST_PRUNE: [
        { operation: 'UPDATE', key: HookStateKeys.HOST_COUNT },
        { operation: 'UPDATE', key: HookStateKeys.REWARD_INFO }

        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    HOST_TRANSFER: [
        // NOTE: Repetitive State keys
        // HookStateKeys.PREFIX_TRANSFEREE_ADDR
        // HookStateKeys.PREFIX_HOST_ADDR
    ],
    HOST_REBATE: [
        // NOTE: Repetitive State keys
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
    #governorClient = null;
    #heartbeatClient = null;
    #registryClient = null;
    #queuedStates = null;

    constructor(rippledServer, governorAddress, stateIndexId = null) {
        this.#xrplApi = new XrplApi(rippledServer);
        Defaults.set({
            governorAddress: governorAddress,
            rippledServer: rippledServer,
            xrplApi: this.#xrplApi,
            networkID: NETWORK_ID
        })
        this.#xrplAcc = new XrplAccount(governorAddress);
        this.#firestoreManager = new FirestoreManager(stateIndexId ? { stateIndexId: stateIndexId } : {});
        this.#queuedStates = [];
    }

    async init(firebaseSecKeyPath) {
        try {
            await this.#xrplApi.connect();
            this.#governorClient = await HookClientFactory.create(HookTypes.governor);
            this.#heartbeatClient = await HookClientFactory.create(HookTypes.heartbeat);
            this.#registryClient = await HookClientFactory.create(HookTypes.registry);

            await this.#connectHooks();
            await this.#subscribeHooks();

            await this.#firestoreManager.authorize(firebaseSecKeyPath);
            this.config = await this.#firestoreManager.getConfigs();
            if (!this.config || !this.config.length) {
                const states = await this.#governorClient.getHookStates();
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
                    await this.#subscribeHooks();
                    await this.#governorClient.on(GovernorEvents.Initialized, async (data) => {
                        await this.#updateStatesKeyQueue({ event: GovernorEvents.Initialized, ...data });
                        await this.#governorClient.connect();
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

        for (const [key, event] of Object.entries(GovernorEvents)) {
            console.log(`Start listening to ${key} event in Governor...`);
            this.#governorClient.on(event, async (data) => { await this.#updateStatesKeyQueue({ event: event, ...data }) });
        }
        for (const [key, event] of Object.entries(RegistryEvents)) {
            console.log(`Start listening to ${key} event in Registry...`);
            this.#registryClient.on(event, async (data) => { await this.#updateStatesKeyQueue({ event: event, ...data }) });
        }
        for (const [key, event] of Object.entries(HeartbeatEvents)) {
            console.log(`Start listening to ${key} event in Heartbeat...`);
            this.#heartbeatClient.on(event, async (data) => { await this.#updateStatesKeyQueue({ event: event, ...data }) });
        }


        console.log(`Listening to transactions (${this.#xrplAcc.address})...`);

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

    // Connect the governor, registry, hearbeat hooks and trying to reconnect in the event of account not found error.
    // Account not found error can be because of a network reset. (Dev and test nets)
    async #connectHooks() {
        let attempts = 0;
        // eslint-disable-next-line no-constant-condition
        while (true) {
            try {
                attempts++;
                let governorConnected = await this.#governorClient.connect();
                let heartbeatConnected = await this.#heartbeatClient.connect();
                let registryConnected = await this.#registryClient.connect();

                if (governorConnected && heartbeatConnected && registryConnected)
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

    // Hooks subscribe method
    async #subscribeHooks() {
        await this.#governorClient.subscribe()
        await this.#heartbeatClient.subscribe()
        await this.#registryClient.subscribe()
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
            const hosts = await this.#governorClient.getAllHosts();
            const configs = await this.#governorClient.getAllConfigs();
            const candidates = await this.#governorClient.getAllCandidates();

            const itemsInIndex = configs.concat(hosts).concat(candidates);
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

    // To get the summary of the listened transaction (effected states with the operation).
    async #getTransactionSummary(data) {
        const trx = data.transaction;
        const event = data.event;
        let affectedStates = [];
        let stateKeyHostAddrId = null;
        let stateKeyTokenId = null;
        let stateKeyCandidateOwner = null;
        let stateKeyCandidateId = null;

        const hostEvents = [
            RegistryEvents.HostRegistered,
            RegistryEvents.HostDeregistered,
            RegistryEvents.HostRegUpdated,
            RegistryEvents.DeadHostPrune,
            RegistryEvents.HostRebate,
            RegistryEvents.HostTransfer,
            HeartbeatEvents.Heartbeat,
            GovernorEvents.DudHostRemoved
        ];

        const candidateEvents = [
            GovernorEvents.CandidateProposed,
            GovernorEvents.CandidateWithdrew,
            GovernorEvents.ChildHookUpdated,
            GovernorEvents.DudHostReported,
            GovernorEvents.DudHostRemoved,
            GovernorEvents.DudHostStatusChanged,
            GovernorEvents.FallbackToPiloted,
            GovernorEvents.NewHookStatusChanged,
            HeartbeatEvents.FoundationVoted
        ];

        console.log(`|${trx.Account}|${event}|Triggered a transaction.`);

        try {
            // HOST_ADDR State Key
            if (hostEvents.includes(event))
                stateKeyHostAddrId = StateHelpers.generateHostAddrStateKey((event === RegistryEvents.DeadHostPrune || event === GovernorEvents.DudHostRemoved) ? data.host : trx.Account);

            // CANDIDATE_ID State Key
            if (candidateEvents.includes(event))
                stateKeyCandidateId = StateHelpers.generateCandidateIdStateKey(data.candidateId);

            switch (event) {
                case GovernorEvents.Initialized:
                    affectedStates = AFFECTED_HOOK_STATE_MAP.INIT.slice();
                    break;
                case RegistryEvents.HostRegistered: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_REG.slice();
                    affectedStates.push({ operation: 'INSERT', key: stateKeyHostAddrId });

                    let transferredTokenId = null;

                    if (trx.Amount.value == EvernodeConstants.NOW_IN_EVRS) {
                        const previousHostRec = await this.#governorClient.getHosts({ futureOwnerAddress: trx.Account });
                        const previousHostAddrStateKey = StateHelpers.generateHostAddrStateKey(previousHostRec[0]?.address);
                        affectedStates.push({ operation: 'DELETE', key: previousHostAddrStateKey });
                        transferredTokenId = previousHostRec[0]?.uriTokenId;
                    }

                    const firstPart = trx.hash.substring(0, 8);
                    const lastPart = trx.hash.substring(trx.hash.length - 8);
                    const trxRef = TransactionHelper.asciiToHex(firstPart + lastPart);

                    const uri = `${EvernodeConstants.TOKEN_PREFIX_HEX}${trxRef}`;
                    let regToken = null;
                    const hostXrplAcc = new XrplAccount(trx.Account);
                    let attempts = 0;
                    while (attempts < NFT_WAIT_TIMEOUT) {
                        // Check in Registry.
                        regToken = (await this.#xrplAcc.getURITokens()).find(n => (n.URI === uri || n.index == transferredTokenId));
                        if (!regToken) {
                            // Check in Host.
                            regToken = (await hostXrplAcc.getURITokens()).find(n => (n.URI === uri));
                        }

                        if (regToken) {
                            break;
                        }

                        await new Promise(r => setTimeout(r, 1000));
                        attempts++;
                    }

                    if (!regToken) {
                        console.log(`|${trx.Account}|${event}|No Reg. Token was found within the timeout.`);
                        break;
                    }

                    stateKeyTokenId = StateHelpers.generateTokenIdStateKey(regToken.index);
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyTokenId });
                    break;
                }
                case RegistryEvents.HostDeregistered:
                    affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_DEREG.slice();
                    affectedStates.push({ operation: 'DELETE', key: stateKeyHostAddrId });
                    break;
                case RegistryEvents.HostRegUpdated: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_UPDATE_REG.slice();
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyHostAddrId });

                    const info = await this.#governorClient.getHostInfo(trx.Account);
                    stateKeyTokenId = StateHelpers.generateTokenIdStateKey(info.uriTokenId);
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyTokenId });

                    break;
                }
                case HeartbeatEvents.Heartbeat: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.HEARTBEAT.slice();
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyHostAddrId });

                    if (data.voteInfo)
                        affectedStates.push({ operation: 'UPDATE', key: StateHelpers.generateCandidateIdStateKey(data.voteInfo.candidateId) });

                    break;
                }
                case HeartbeatEvents.FoundationVoted: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.CANDIDATE_VOTE.slice();
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyCandidateId });
                    break;
                }
                case RegistryEvents.DeadHostPrune: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.DEAD_HOST_PRUNE.slice();
                    affectedStates.push({ operation: 'DELETE', key: stateKeyHostAddrId });
                    break;
                }
                case RegistryEvents.HostRebate: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_REBATE.slice();
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyHostAddrId });
                    break;
                }
                case RegistryEvents.HostTransfer: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.HOST_TRANSFER.slice();
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyHostAddrId });
                    const transfereeAddressKey = StateHelpers.generateTransfereeAddrStateKey(data.transferee);
                    affectedStates.push({ operation: 'UPDATE', key: transfereeAddressKey });
                    break;
                }
                case GovernorEvents.CandidateProposed: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.CANDIDATE_PROPOSE.slice();
                    affectedStates.push({ operation: 'INSERT', key: stateKeyCandidateId });
                    stateKeyCandidateOwner = StateHelpers.generateCandidateOwnerStateKey(data.owner);
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyCandidateOwner });
                    break;
                }
                case GovernorEvents.CandidateWithdrew: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.CANDIDATE_WITHDRAW.slice();
                    affectedStates.push({ operation: 'DELETE', key: stateKeyCandidateId });
                    break;
                }
                case GovernorEvents.DudHostRemoved: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.DUD_HOST_REMOVE.slice();
                    affectedStates.push({ operation: 'DELETE', key: stateKeyHostAddrId });
                    affectedStates.push({ operation: 'DELETE', key: stateKeyCandidateId });
                    break;
                }
                case GovernorEvents.DudHostStatusChanged: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.DUD_HOST_STATUS_CHANGE.slice();
                    affectedStates.push({ operation: 'DELETE', key: stateKeyCandidateId });
                    break;
                }
                case GovernorEvents.FallbackToPiloted: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.FALLBACK_PILOTED.slice();
                    affectedStates.push({ operation: 'DELETE', key: stateKeyCandidateId });
                    break;
                }
                case GovernorEvents.NewHookStatusChanged: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.CANDIDATE_ELECT.slice();
                    affectedStates.push({ operation: 'DELETE', key: stateKeyCandidateId });
                    break;
                }
                case GovernorEvents.ChildHookUpdated: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.CHILD_HOOK_UPDATE.slice();
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyCandidateId });
                    break;
                }
                case GovernorEvents.GovernanceModeChanged: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.CHANGE_GOVERNANCE_MODE.slice();

                    if (data.mode === EvernodeConstants.GovernanceModes.CoPiloted || data.mode === EvernodeConstants.GovernanceModes.AutoPiloted)
                        affectedStates.push({ operation: 'INSERT', key: StateHelpers.generateCandidateIdStateKey(StateHelpers.getPilotedModeCandidateId()) });

                    break;
                }
                case GovernorEvents.DudHostReported: {
                    affectedStates = AFFECTED_HOOK_STATE_MAP.DUD_HOST_REPORT.slice();
                    affectedStates.push({ operation: 'UPDATE', key: stateKeyCandidateId });
                    break;
                }
            }

            if (event != GovernorEvents.Initialized)
                affectedStates.push(...AFFECTED_HOOK_STATE_MAP.COMMON);

            console.log(`|${trx.Account}|${event}|Completed fetching transaction data`);
        }
        catch (e) {
            console.error(e);
        }

        return affectedStates
    }

    #prepareHostObject(decodedObj) {
        // If this is a token id update we replace the key with address key,
        // So the existing host address state will get updated.
        if (decodedObj.type == StateHelpers.StateTypes.TOKEN_ID) {
            decodedObj.key = decodedObj.addressKey;
            delete decodedObj.addressKey;
        }

        // If this is a transferee addr state update we add additional data to the host info.
        // So the existing host address state will get updated.
        if (decodedObj.type == StateHelpers.StateTypes.TRANSFEREE_ADDR) {
            decodedObj.key = decodedObj.prevHostAddressKey;
            delete decodedObj.prevHostAddress;
            delete decodedObj.prevHostAddressKey;
            delete decodedObj.transferredNfTokenId;
        }

        delete decodedObj.type;

        return decodedObj;
    }

    #prepareCandidateObject(decodedObj) {
        // If this is a candidate owner update we replace the key with candidate id key,
        if (decodedObj.type == StateHelpers.StateTypes.CANDIDATE_OWNER) {
            decodedObj.key = decodedObj.idKey;
            delete decodedObj.idKey;
        }

        delete decodedObj.type;

        return decodedObj;
    }

    // To update the Firestore index with a set of pending states in the queue.
    async #updateIndexStates(processingStates) {
        const inserts = [];
        const updates = [];
        const deletes = [];
        const hookStates = await this.#governorClient.getHookStates();

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
            else if (decoded.type == StateHelpers.StateTypes.CANDIDATE_OWNER || decoded.type == StateHelpers.StateTypes.CANDIDATE_ID)
                await this.#firestoreManager.setCandidate(this.#prepareCandidateObject(decoded));
            else
                await this.#firestoreManager.setHost(this.#prepareHostObject(decoded));
        }));

        await Promise.all(updates.map(async ps => {
            const decoded = StateHelpers.decodeStateData(Buffer.from(ps.key, 'hex'), Buffer.from(ps.data, 'hex'));
            if (decoded.type == StateHelpers.StateTypes.SIGLETON || decoded.type == StateHelpers.StateTypes.CONFIGURATION)
                await this.#firestoreManager.setConfig(decoded, true);
            else if (decoded.type == StateHelpers.StateTypes.CANDIDATE_OWNER || decoded.type == StateHelpers.StateTypes.CANDIDATE_ID)
                await this.#firestoreManager.setCandidate(this.#prepareCandidateObject(decoded), true);
            else
                await this.#firestoreManager.setHost(this.#prepareHostObject(decoded), true);
        }));

        await Promise.all(deletes.map(async ps => {
            const decoded = StateHelpers.decodeStateKey(Buffer.from(ps.key, 'hex'));
            if (decoded.type == StateHelpers.StateTypes.SIGLETON || decoded.type == StateHelpers.StateTypes.CONFIGURATION)
                await this.#firestoreManager.deleteConfig(ps.key);
            else if (decoded.type == StateHelpers.StateTypes.CANDIDATE_ID)
                await this.#firestoreManager.deleteCandidate(decoded.key);
            else if (decoded.type == StateHelpers.StateTypes.HOST_ADDR)
                await this.#firestoreManager.deleteHost(decoded.key);
        }));
    }

    // To update the Firestore index in a recovery situation.
    async #recoveryIndexUpdate(affectedStates) {

        const hookStates = await this.#governorClient.getHookStates();
        if (!hookStates) {
            throw "No state entries were found for this hook account";
        }

        // To find removed states.
        const hookStateKeys = hookStates.map(s => s.key);
        const removedStates = (affectedStates.filter(s => !(hookStateKeys.includes(s)))).map(st => { return { key: st } });

        let indexUpdates = {
            set: {
                hosts: {},
                configs: {},
                candidates: {}
            },
            delete: {
                hosts: {},
                configs: {},
                candidates: {}
            }
        };

        const updateIndexSet = (key, value) => {
            const decoded = StateHelpers.decodeStateData(key, value);
            // If the object already exists we override it.
            // We combine host address and token objects.
            if (decoded.type == StateHelpers.StateTypes.HOST_ADDR || decoded.type == StateHelpers.StateTypes.TOKEN_ID || decoded.type == StateHelpers.StateTypes.TRANSFEREE_ADDR) {
                const prepared = this.#prepareHostObject(decoded);
                indexUpdates.set.hosts[prepared.key] = {
                    ...(indexUpdates.set.hosts[prepared.key] ? indexUpdates.set.hosts[prepared.key] : {}),
                    ...prepared
                }
            }
            else if (decoded.type == StateHelpers.StateTypes.CANDIDATE_OWNER || decoded.type == StateHelpers.StateTypes.CANDIDATE_ID) {
                const prepared = this.#prepareCandidateObject(decoded);
                indexUpdates.set.candidates[prepared.key] = {
                    ...(indexUpdates.set.candidates[prepared.key] ? indexUpdates.set.candidates[prepared.key] : {}),
                    ...prepared
                }
            }
            else if (decoded.type == StateHelpers.StateTypes.SIGLETON || decoded.type == StateHelpers.StateTypes.CONFIGURATION)
                indexUpdates.set.configs[decoded.key] = decoded;
        }

        const deleteIndexSet = (key) => {
            const decoded = StateHelpers.decodeStateKey(key);

            if (decoded.type == StateHelpers.StateTypes.HOST_ADDR)
                indexUpdates.delete.hosts[decoded.key] = true;
            else if (decoded.type == StateHelpers.StateTypes.CANDIDATE_ID)
                indexUpdates.delete.candidates[decoded.key] = true;
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
                ...Object.values(indexUpdates.set.candidates).map(async obj => {
                    await this.#firestoreManager.setCandidate(obj);
                }),
                ...Object.values(indexUpdates.set.configs).map(async obj => {
                    await this.#firestoreManager.setConfig(obj);
                }),
                ...Object.keys(indexUpdates.delete.hosts).map(async key => {
                    await this.#firestoreManager.deleteHost(key);
                }),
                ...Object.keys(indexUpdates.delete.candidates).map(async key => {
                    await this.#firestoreManager.deleteCandidate(key);
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
    let memoData = Buffer.allocUnsafe(80);
    codec.decodeAccountID(config.issuer.address).copy(memoData);
    codec.decodeAccountID(config.foundation.address).copy(memoData, 20);
    codec.decodeAccountID(config.registry.address).copy(memoData, 40);
    codec.decodeAccountID(config.heartbeat.address).copy(memoData, 60);

    const xrplApi = new XrplApi(rippledServer);
    await xrplApi.connect();
    const initAccount = new XrplAccount(initializerInfo.address, initializerInfo.secret, { xrplApi: xrplApi });
    const res = await initAccount.makePayment(config.governor.address, MIN_XRP, 'XRP', null,
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
    // Governor address is required as a command line param.
    if (process.argv.length != 3 || !process.argv[2]) {
        console.error('Registry address is required as a command line parameter.');
        return;
    }
    const governorAddress = process.argv[2];
    const configPath = path.resolve(DATA_DIR, CONFIG_FILE);
    const accountConfigPath = path.resolve(HOOK_DATA_DIR, governorAddress, ACCOUNT_CONFIG_FILE);

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
    else if (!accountConfig.governor || !accountConfig.governor.address || !accountConfig.governor.secret) {
        console.error('Registry account info not found, run the account setup tool and generate the accounts.');
        return;
    }

    // Firestore Service Account key
    if (!fs.existsSync(FIREBASE_SEC_KEY_PATH)) {
        console.error(`${FIREBASE_SEC_KEY_PATH} not found, place the json file in the location.`);
        return;
    }

    // Send the accountConfig init transaction to the governor account.
    if (!accountConfig.initialized) {
        console.log('Sending governor contract initialization transation.');
        const res = await initRegistryConfigs(config.hookInitializer, accountConfig, accountConfigPath, RIPPLED_URL).catch(e => {
            throw { message: 'Registry contract initialization transaction failed with.', error: e.hookExecutionResult };
        });
        if (res)
            console.log('Registry contract initialization success.')
    }

    // Start the Index Manager.
    const indexManager = new IndexManager(RIPPLED_URL, accountConfig.governor.address, MODE === 'beta' ? BETA_STATE_INDEX : null);
    await indexManager.init(FIREBASE_SEC_KEY_PATH);
}

main().catch(e => {
    console.error(e);
    process.exit(1);
});
