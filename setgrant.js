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
        },
        "heartbeat": {
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
const heartbeatAddress = cfg.heartbeat.address;

if (!registryAddress || !governorSecret) {
    console.error("SETHOOK FAILED: Please specify governor secret and registry address in hook.json");
    process.exit(1);
}
else {
    const account = xrpljs.Wallet.fromSeed(governorSecret);
    getHookHashes(account.classicAddress).then(hookHashes => {
        hookHashes = hookHashes.filter(h => h);
        getHookHashes(registryAddress).then(hook2Hashes => {
            hook2Hashes = hook2Hashes.map(h => ({address: registryAddress, hash:h}));
            getHookHashes(heartbeatAddress).then(hook3Hashes => {
                hook3Hashes = hook3Hashes.map(h => ({address: heartbeatAddress, hash:h}));
                if (hookHashes && hookHashes.length && hook2Hashes && hook2Hashes.length && hook3Hashes && hook3Hashes.length) {
                    const grantHashes = [...hook2Hashes, ...hook3Hashes];
                    const hookTx = {
                        Account: account.classicAddress,
                        TransactionType: "SetHook",
                        Hooks: hookHashes.map(() => {
                            return {
                                Hook: {
                                    HookGrants: grantHashes.map(h => {
                                        return {
                                            HookGrant:
                                            {
                                                Authorize: h.address,
                                                HookHash: h.hash
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
    });
}