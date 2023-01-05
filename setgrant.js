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
    let hookHashes = await getHookHashes(account.classicAddress);
    
    let hook2Hashes = await getHookHashes(registryAddress);
    let hook3Hashes = await getHookHashes(heartbeatAddress);

    const hookTx = {
        Account: account.classicAddress,
        TransactionType: "SetHook",
        Hooks: hookHashes.map(() => {
            return {
                Hook: {
                    HookGrants: [{
                        ...hook2Hashes.map(h => ({ HookGrant: { Authorize: registryAddress, HookHash: h } })),
                        ...hook3Hashes.map(h => ({ HookGrant: { Authorize: heartbeatAddress, HookHash: h } }))
                    }]
                }
            };
        })
    };

    submitTxn(governorSecret, hookTx).then(res => { console.log(res); }).catch(console.error).finally(() => process.exit(0));

}