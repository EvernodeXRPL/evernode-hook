const fs = require('fs');
const process = require('process');
const xrpljs = require('xrpl-hooks');
const { submitTxn, getHookHashes, appenv, init } = require('./common');

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
        },
        "reputation": {
            "address": "",
            "secret": ""
        },
        "network": ""
    }
    fs.writeFileSync(CONFIG_PATH, JSON.stringify(cfg, null, 2));
}
else {
    cfg = JSON.parse(fs.readFileSync(CONFIG_PATH));
}

const governorSecret = cfg.governor.secret;
const registryAddress = cfg.registry.address;
const heartbeatAddress = cfg.heartbeat.address;
const reputationAddress = cfg.reputation.address;

if (!registryAddress || !heartbeatAddress || !reputationAddress || !governorSecret) {
    console.error("SETHOOK FAILED: Please specify registry, heartbeat, reputation address and governor secret in hook.json");
    process.exit(1);
}
else {
    init(cfg.network).then(() => {
        const account = xrpljs.Wallet.fromSeed(governorSecret);
        getHookHashes(account.classicAddress).then(async hookHashes => {
            let hook2Hashes = await getHookHashes(registryAddress);
            let hook3Hashes = await getHookHashes(heartbeatAddress);
            let hook4Hashes = await getHookHashes(reputationAddress);

            if (hookHashes && hookHashes.length && hook2Hashes && hook2Hashes.length && hook3Hashes && hook3Hashes.length && hook4Hashes && hook4Hashes.length) {
                const hookTx = {
                    Account: account.classicAddress,
                    TransactionType: "SetHook",
                    NetworkID: appenv.NETWORK_ID,
                    Hooks: hookHashes.map(() => {
                        return {
                            Hook: {
                                HookGrants: [
                                    ...hook2Hashes.map(h => ({ HookGrant: { Authorize: registryAddress, HookHash: h } })),
                                    ...hook3Hashes.map(h => ({ HookGrant: { Authorize: heartbeatAddress, HookHash: h } })),
                                    ...hook4Hashes.map(h => ({ HookGrant: { Authorize: reputationAddress, HookHash: h } }))
                                ]
                            }
                        };
                    })
                };

                submitTxn(governorSecret, hookTx).then(res => { console.log(res); }).catch(console.error).finally(() => process.exit(0));
            } else {
                console.error("Error in fetching hook hashes.");
                process.exit(1);
            }
        });
    }).catch(console.error);
}