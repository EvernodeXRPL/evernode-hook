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

const governorSecret = cfg.governor.secret;
const registryAddress = cfg.registry.address;

if (!registryAddress || !governorSecret) {
    console.error("SETHOOK FAILED: Please specify governor secret and registry address in hook.json");
    process.exit(1);
}
else {
    const account = xrpljs.Wallet.fromSeed(governorSecret);
    getHookHashes(account.classicAddress).then(hookHashes => {
        hookHashes = hookHashes.filter(h => h);
        getHookHashes(registryAddress).then(hook2Hashes => {
            hook2Hashes = hook2Hashes.filter(h => h);
            if (hookHashes && hookHashes.length && hook2Hashes && hook2Hashes.length) {
                const hookTx = {
                    Account: account.classicAddress,
                    TransactionType: "SetHook",
                    Hooks: hookHashes.map(() => {
                        return {
                            Hook: {
                                HookGrants: hook2Hashes.map(h2 => {
                                    return {
                                        HookGrant:
                                        {
                                            Authorize: registryAddress,
                                            HookHash: h2
                                        }
                                    }
                                })
                            }
                        }
                    })
                };
                submitTxn(governorSecret, hookTx).then(res => { console.log(res); }).catch(console.error).finally(() => process.exit(0))
            }
        });
    });
}