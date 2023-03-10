const fs = require('fs');
const process = require('process');
const xrpljs = require('xrpl-hooks');
const { submitTxn, appenv } = require('../common');

const hsfOVERRIDE = appenv.hsfOVERRIDE;
const hsfNSDELETE = appenv.hsfNSDELETE;

const NAMESPACE = appenv.NAMESPACE;

const CONFIG_PATH = appenv.CONFIG_PATH;

const WASM_PATH_GOVERNOR = `${appenv.WASM_DIR_PATH}/governor.wasm`;

console.log(WASM_PATH_GOVERNOR);

let cfg;

if (!fs.existsSync(CONFIG_PATH)) {
    cfg = {
        "governor": {
            "address": "",
            "secret": ""
        }
    }
    fs.writeFileSync(CONFIG_PATH, JSON.stringify(cfg, null, 2));
}
else {
    cfg = JSON.parse(fs.readFileSync(CONFIG_PATH));
}

const governorSecret = cfg.governor.secret;

if (!governorSecret) {
    console.error("SETHOOK FAILED: Please specify the governor secret in hook.json");
    process.exit(1);
}
else {
    const account = xrpljs.Wallet.fromSeed(governorSecret)
    const binaryZero = fs.readFileSync(WASM_PATH_GOVERNOR).toString('hex').toUpperCase();

    let hookTx = {
        Account: account.classicAddress,
        TransactionType: "SetHook",
        NetworkID: appenv.NETWORK_ID,
        Hooks:
            [{
                Hook: {
                    CreateCode: binaryZero.slice(0, 194252),
                    HookOn: '0000000000000000000000000000000000000000000000000000000000000000',
                    HookNamespace: NAMESPACE,
                    HookApiVersion: 0,
                    Flags: hsfOVERRIDE
                }
            },
            { Hook: { Flags: hsfOVERRIDE || hsfNSDELETE, CreateCode: '' } },
            { Hook: { Flags: hsfOVERRIDE || hsfNSDELETE, CreateCode: '' } },
            { Hook: { Flags: hsfOVERRIDE || hsfNSDELETE, CreateCode: '' } }]
    };

    submitTxn(governorSecret, hookTx).then(res => { console.log(res); }).catch(console.error).finally(() => process.exit(0))
}