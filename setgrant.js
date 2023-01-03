const fs = require('fs');
const process = require('process');
const xrpljs = require('xrpl-hooks');
const { submitTxn, getHookHashes, appenv } = require('./common');

const CONFIG_PATH = appenv.CONFIG_PATH;

let cfg;

if (!fs.existsSync(CONFIG_PATH)) {
    cfg = {
        "governor": {
            "address": "",
            "secret": ""
        },
        "registry": {
            "address": "",
            "secret": ""
        }
    }
    fs.writeFileSync(CONFIG_PATH, JSON.stringify(cfg, null, 2));
}
else {
    cfg = JSON.parse(fs.readFileSync(CONFIG_PATH));
}

const registrySecret = cfg.registry.secret;
const governorAddress = cfg.governor.address;

if (!registrySecret || !governorAddress) {
    console.error("SETHOOK FAILED: Please specify registry secret and governor address in hook.json");
    process.exit(1);
}
else {
    getHookHashes(governorAddress).then(hashes => {
        if (hashes && hashes.length) {
            const account = xrpljs.Wallet.fromSeed(registrySecret)
            const hookTx = {
                Account: account.classicAddress,
                TransactionType: "SetHook",
                Hooks:
                    [
                        {
                            Hook: {
                                HookGrants: hashes.filter(h => h).map(h => {
                                    return {
                                        HookGrant:
                                        {
                                            Authorize: governorAddress,
                                            HookHash: h
                                        }
                                    }
                                })
                            }
                        }
                    ]
            };
            submitTxn(registrySecret, hookTx).then(res => { console.log(res); }).catch(console.error).finally(() => process.exit(0))
        }
    });
}