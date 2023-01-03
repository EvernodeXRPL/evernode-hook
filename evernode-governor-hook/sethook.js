const fs = require('fs');
const process = require('process');
const xrpljs = require('xrpl-hooks');
const codec = require('ripple-address-codec');
const { submitTxn, appenv } = require('../common');

const hsfOVERRIDE = appenv.hsfOVERRIDE;

const NAMESPACE = appenv.NAMESPACE;
const PARAM_STATE_HOOK_KEY = appenv.PARAM_STATE_HOOK_KEY;

const CONFIG_PATH = appenv.CONFIG_PATH;

const WASM_PATH_ZERO = `${appenv.WASM_DIR_PATH}/evernodezero.wasm`;
const WASM_PATH_ONE = `${appenv.WASM_DIR_PATH}/evernodeone.wasm`;
const WASM_PATH_TWO = `${appenv.WASM_DIR_PATH}/evernodetwo.wasm`;

console.log(WASM_PATH_ZERO, WASM_PATH_ONE, WASM_PATH_TWO);

let cfg;

if (!fs.existsSync(CONFIG_PATH)) {
    cfg = {
        "governor": {
            "address": "",
            "secret": ""
        },
        "registry": {
            "address": ""
        }
    }
    fs.writeFileSync(CONFIG_PATH, JSON.stringify(cfg, null, 2));
}
else {
    cfg = JSON.parse(fs.readFileSync(CONFIG_PATH));
}

const governorSecret = cfg.governor.secret;
const registryAddress = cfg.registry.address;

if (!governorSecret || !registryAddress) {
    console.error("SETHOOK FAILED: Please specify the governor secret and registry address in hook.json");
    process.exit(1);
}
else {
    const account = xrpljs.Wallet.fromSeed(governorSecret)
    const binaryZero = fs.readFileSync(WASM_PATH_ZERO).toString('hex').toUpperCase();
    const binaryOne = fs.readFileSync(WASM_PATH_ONE).toString('hex').toUpperCase();
    const binaryTwo = fs.readFileSync(WASM_PATH_TWO).toString('hex').toUpperCase();
    const registryAccId = codec.decodeAccountID(registryAddress).toString('hex').toUpperCase();

    let hookTx = {
        Account: account.classicAddress,
        TransactionType: "SetHook",
        Hooks:
            [
                {
                    Hook: {
                        CreateCode: binaryZero.slice(0, 194252),
                        HookOn: '0000000000000000',
                        HookNamespace: NAMESPACE,
                        HookApiVersion: 0,
                        Flags: hsfOVERRIDE,
                        HookParameters:
                            [
                                {
                                    HookParameter:
                                    {
                                        HookParameterName: PARAM_STATE_HOOK_KEY,
                                        HookParameterValue: registryAccId
                                    }
                                }
                            ]
                    }
                },
                {
                    Hook: {
                        CreateCode: binaryOne.slice(0, 194252),
                        HookOn: '0000000000000000',
                        HookNamespace: NAMESPACE,
                        HookApiVersion: 0,
                        Flags: hsfOVERRIDE,
                        HookParameters:
                            [
                                {
                                    HookParameter:
                                    {
                                        HookParameterName: PARAM_STATE_HOOK_KEY,
                                        HookParameterValue: registryAccId
                                    }
                                }
                            ]
                    }
                },
                {
                    Hook: {
                        CreateCode: binaryTwo.slice(0, 194252),
                        HookOn: '0000000000000000',
                        HookNamespace: NAMESPACE,
                        HookApiVersion: 0,
                        Flags: hsfOVERRIDE,
                        HookParameters:
                            [
                                {
                                    HookParameter:
                                    {
                                        HookParameterName: PARAM_STATE_HOOK_KEY,
                                        HookParameterValue: registryAccId
                                    }
                                }
                            ]
                    }
                }
            ]
    };

    submitTxn(governorSecret, hookTx).then(res => { console.log(res); }).catch(console.error).finally(() => process.exit(0))
}