const fs = require('fs');
const evernode = require("evernode-js-client");

const CONFIG_PATH = 'accounts.json';
const TOTAL_MINTED_EVRS = "72253440";
const NETWORK_ID = 21338;

const ALL = 0;
const MIGRATE = 1;
const CLEAR = 2;

let mode = ALL;
if (process.argv.length > 2) {
    if (process.argv[2] == "migrate")
        mode = MIGRATE;
    else if (process.argv[2] == "clear")
        mode = CLEAR;
}

const cfg = JSON.parse(fs.readFileSync(CONFIG_PATH));

const registryAddress = cfg.registry.address;
const registrySecret = cfg.registry.secret;
const heartbeatAddress = cfg.heartbeat.address;
const heartbeatSecret = cfg.heartbeat.secret;
const governorAddress = cfg.governor.address;
const governorSecret = cfg.governor.secret;
const migratorAddress = cfg.migrator.address;
const migratorSecret = cfg.migrator.secret;

async function main() {
    const xrplApi = new evernode.XrplApi(cfg.server);
    evernode.Defaults.set({
        registryAddress: registryAddress,
        xrplApi: xrplApi,
        NetworkID: NETWORK_ID
    })

    try {
        await xrplApi.connect();

        if (mode == ALL || mode == MIGRATE) {
            await migrateStates();
            await transferEVRs();
        }
        if (mode == ALL || mode == CLEAR)
            await clearSourceStates();
    }
    catch (e) {
        console.error("Error occured:", e);
    }
    finally {
        await xrplApi.disconnect();
    }
}

async function getSourceStates() {
    const client = new evernode.RegistryClient();
    try {
        await client.connect();
        const res = await client.getHookStates();
        await client.disconnect();
        return res;
    }
    catch {
        await client.disconnect();
        return [];
    }
}

async function getDestinationStates() {
    const client = new evernode.RegistryClient({ registryAddress: governorAddress });
    try {
        await client.connect();
        const res = await client.getHookStates();
        await client.disconnect();
        return res;
    }
    catch {
        await client.disconnect();
        return [];
    }
}

async function getDeSyncedStates() {
    const sourceStates = await getSourceStates();
    const destinationStates = await getDestinationStates();
    let res;
    if (sourceStates && destinationStates)
        res = sourceStates.filter(s1 => !destinationStates.find(s2 => s1.key === s2.key && s1.data === s2.data));
    else
        res = [];
    return {
        sourceStates: sourceStates,
        destinationStates: destinationStates,
        deSyncedStates: res
    }
}

async function migrateStates() {
    console.log('Getting de-synced states');
    const states = (await getDeSyncedStates()).deSyncedStates;

    console.log(`Found ${states.length} de-synced states`);

    console.log('Sending migration payments');
    const initAccount = new evernode.XrplAccount(migratorAddress, migratorSecret);
    for (const state of states) {
        await initAccount.makePayment(governorAddress, '1', 'XRP', null,
            [{ type: 'state', format: 'hex', data: `${state.key}${state.data}` }]);
        console.log(`Migrated state ${state.key}`);
    }

    const deSynced = (await getDeSyncedStates()).deSyncedStates;
    if (!deSynced || !deSynced.length)
        console.log(`All states are synced!. Destination has ${(await getDestinationStates()).length} out of ${(await getSourceStates()).length} states.`);
    else
        console.error('State data has not migrated or invalid migration');
}

async function clearSourceStates() {
    const sourceStates = await getSourceStates();
    console.log(`Found ${sourceStates.length} states`);

    console.log('Sending clear payments');
    const initAccount = new evernode.XrplAccount(migratorAddress, migratorSecret);
    for (const state of sourceStates) {
        await initAccount.makePayment(registryAddress, '1', 'XRP', null,
            [{ type: 'state', format: 'hex', data: `${state.key}` }]);
        console.log(`Cleared state ${state.key}`);
    }

    const clearedStates = await getSourceStates();
    if (!clearedStates || !clearedStates.length)
        console.log('All states are cleared!');
    else
        console.error('State data has not cleared');
}

async function transferEVRs() {
    // Transfer EVRs to heartbeat.
    // Check trustlines.
    const heartbeatAccount = new evernode.XrplAccount(heartbeatAddress, heartbeatSecret);
    const governorAccount = new evernode.XrplAccount(governorAddress, governorSecret);
    const registryClient = new evernode.RegistryClient({ registryAddress: registryAddress, });

    try {
        await registryClient.connect();
    
        const heartbeatLines = await heartbeatAccount.getTrustLines(evernode.EvernodeConstants.EVR, registryClient.config.evrIssuerAddress);
        if (heartbeatLines.length === 0)
            await heartbeatAccount.setTrustLine(evernode.EvernodeConstants.EVR, registryClient.config.evrIssuerAddress, TOTAL_MINTED_EVRS);
    
        const governorLines = await governorAccount.getTrustLines(evernode.EvernodeConstants.EVR, registryClient.config.evrIssuerAddress);
        if (governorLines.length === 0)
            await governorAccount.setTrustLine(evernode.EvernodeConstants.EVR, registryClient.config.evrIssuerAddress, TOTAL_MINTED_EVRS);
    
        const remainingEpochs = registryClient.config.rewardConfiguration.epochCount - registryClient.config.rewardInfo.epoch;
        const rewardAmount = (remainingEpochs * registryClient.config.rewardConfiguration.epochRewardAmount) + parseFloat(registryClient.config.rewardInfo.epochPool);

        const regAcc = new evernode.XrplAccount(registryAddress, registrySecret);
        await regAcc.makePayment(heartbeatAddress, rewardAmount.toString(), evernode.EvernodeConstants.EVR, registryClient.config.evrIssuerAddress);
    
        await registryClient.disconnect();
        console.error('Transferred reward EVRs to the heartbeat hook account.');
    }
    catch (e) {
        await registryClient.disconnect();
        throw e;
    }
}

main().catch(console.error).finally(() => process.exit(0));