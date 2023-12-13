const fs = require('fs');
const process = require('process');
const xrpljs = require('xrpl-hooks');
const codec = require('ripple-address-codec');
const { submitTxn, appenv, init } = require('../common');

const hsfOVERRIDE = appenv.hsfOVERRIDE;
const hsfNSDELETE = appenv.hsfNSDELETE;

const NAMESPACE = appenv.NAMESPACE;
const PARAM_STATE_HOOK_KEY = appenv.PARAM_STATE_HOOK_KEY;

const CONFIG_PATH = appenv.CONFIG_PATH;

const WASM_PATH = `${appenv.WASM_DIR_PATH}/heartbeat.wasm`;

console.log(WASM_PATH);

let cfg;

if (!fs.existsSync(CONFIG_PATH)) {
    cfg = {
        "heartbeat": {
            "address": "",
            "secret": ""
        },
        "governor": {
            "address": ""
        },
        "network": ""
    }
    fs.writeFileSync(CONFIG_PATH, JSON.stringify(cfg, null, 2));
}
else {
    cfg = JSON.parse(fs.readFileSync(CONFIG_PATH));
}

const heartbeatSecret = cfg.heartbeat.secret;
const governorAddress = cfg.governor.address;

if (!heartbeatSecret || !governorAddress) {
    console.error("SETHOOK FAILED: Please specify the heartbeat secret and governor address in hook.json");
    process.exit(1);
}
else {
    init(cfg.network).then(() => {
        const account = xrpljs.Wallet.fromSeed(heartbeatSecret)
        const binary = fs.readFileSync(WASM_PATH).toString('hex').toUpperCase();
        const governorAccId = codec.decodeAccountID(governorAddress).toString('hex').toUpperCase();

        const hookTx = {
            Account: account.classicAddress,
            TransactionType: "SetHook",
            NetworkID: appenv.NETWORK_ID,
            Hooks:
                [{
                    Hook: {
                        CreateCode: binary,
                        HookOn: '0000000000000000000000000000000000000000000000000000000000000000',
                        HookNamespace: NAMESPACE,
                        HookApiVersion: 0,
                        Flags: hsfOVERRIDE,
                        HookParameters:
                            [{
                                HookParameter:
                                {
                                    HookParameterName: PARAM_STATE_HOOK_KEY,
                                    HookParameterValue: governorAccId
                                }
                            }]
                    }
                },
                { Hook: { Flags: hsfOVERRIDE || hsfNSDELETE, CreateCode: '' } },
                { Hook: { Flags: hsfOVERRIDE || hsfNSDELETE, CreateCode: '' } },
                { Hook: { Flags: hsfOVERRIDE || hsfNSDELETE, CreateCode: '' } }]
        };

        submitTxn(heartbeatSecret, hookTx).then(res => { console.log(res); }).catch(console.error).finally(() => process.exit(0))
    }).catch(console.error);
}