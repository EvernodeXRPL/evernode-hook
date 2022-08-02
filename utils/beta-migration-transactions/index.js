
const process = require('process');
const fs = require('fs');
const codec = require('ripple-address-codec');
const { XrplAccount, XrplApi, EvernodeConstants, RegistryClient } = require('evernode-js-client');

const MIN_XRP = "1";
const INIT_MEMO_TYPE = "evnInitialize";
const INIT_MEMO_FORMAT = "hex";
const CONFIG_FILE = './evr-transfer-config.json';

// Hooks V2 TEST NET URL.
const RIPPLED_URL = process.env.CONF_RIPPLED_URL || 'wss://hooks-testnet-v2.xrpl-labs.com';

function readConfigs() {
    // If config doesn't exist, skip the execution.
    if (!fs.existsSync(CONFIG_FILE)) {
        console.error(`${CONFIG_FILE} not found,`);
        return;
    }
    let config = JSON.parse(fs.readFileSync(CONFIG_FILE).toString());

    // If required files and config data doesn't exist, skip the execution.
    if (!config.registry.address || !config.initializer.secret ||
        !config.initializer.address || !config.purchaser.address || !config.purchaser.secret) {
        throw "File does not include necessary configs.";
    }

    return config;
}

// This method is used to tranfer EVR from purchaser wallet to registry wallet
async function transferEvrToRegistry(config) {
    const xrplApi = new XrplApi(RIPPLED_URL);
    await xrplApi.connect();

    const regClient = new RegistryClient(config.registry.address);
    await regClient.connect();

    // Get issuer and foundation cold wallet account ids.
    let memoData = Buffer.allocUnsafe(40);
    codec.decodeAccountID(regClient.configs.issuerAddress).copy(memoData);
    codec.decodeAccountID(config.purchaser.address).copy(memoData, 20);

    const purchaserWallet = new XrplAccount(config.purchaser.address, config.purchaser.secret, { xrplApi: xrplApi });
    res = await purchaserWallet.makePayment(config.registry.address, '51609600', EvernodeConstants.EVR, regClient.configs.issuerAddress);
    if (res.code !== 'tesSUCCESS')
        throw res;

    const initAccount = new XrplAccount(config.initializer.address, config.initializer.secret, { xrplApi: xrplApi });
    let res = await initAccount.makePayment(config.registry.address, MIN_XRP, 'XRP', null,
        [{ type: INIT_MEMO_TYPE, format: INIT_MEMO_FORMAT, data: memoData.toString('hex') }]);

    if (res.code === 'tesSUCCESS')
        return res;
    else {
        throw res;
    }
}

async function main() {
    let config = readConfigs();
    await transferEvrToRegistry(config);
}

main().catch(e => {
    console.error('Error:', e);
    process.exit(1);
});