const fs = require('fs');
const xrpljs = require('xrpl-hooks');
const rbc = require('xrpl-binary-codec');

const hsfOVERRIDE = 1
const hsfNSDELETE = 2;

// sha256('evernode.org|registry')
const NAMESPACE = '01EAF09326B4911554384121FF56FA8FECC215FDDE2EC35D9E59F2C53EC665A0'

const CONFIG_PATH = 'accounts.json';
const WASM_PATH = "build/hook.wasm";

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

if (!fs.existsSync(CONFIG_PATH)) {
    fs.writeFileSync(CONFIG_PATH, JSON.stringify({
        "registry": {
            "address": "",
            "secret": ""
        },
        "heartbeat": {
            "address": "",
            "secret": ""
        },
        "governor": {
            "address": "",
            "secret": ""
        },
        "migrator": {
            "address": "rQrU4ae2saKA9toH8U2SYhrJFkwkNiwnvf",
            "secret": "ssd2Q4p2e4FhZ9ActsSTuiKrDFBdR"
        },
        "server": "wss://hooks-testnet-v3.xrpl-labs.com"
    }));
    console.log('Populate the configs and run again!');
    process.exit(0);
}

const cfg = JSON.parse(fs.readFileSync(CONFIG_PATH));

const governorSecret = cfg.governor.secret;
const registrySecret = cfg.registry.secret;

if (!governorSecret) {
    console.error("SETHOOK FAILED: Please specify governor secret in accounts.json");
    process.exit(1);
}
else if (!registrySecret) {
    console.error("SETHOOK FAILED: Please specify registry secret in accounts.json");
    process.exit(1);
}


const api = new xrpljs.Client(cfg.server);

const fee = (txBlob) => {
    return new Promise((resolve, reject) => {
        let req = { command: 'fee' };
        if (txBlob)
            req['tx_blob'] = txBlob;

        api.request(req).then(resp => {
            resolve(resp.result.drops);
        }).catch(e => {
            reject(e);
        });
    });
};

const feeCompute = (accountSeed, txnOrg) => {
    return new Promise((resolve, reject) => {
        let txnToSend = { ...txnOrg };
        txnToSend['SigningPubKey'] = '';

        let wal = xrpljs.Wallet.fromSeed(accountSeed);
        api.prepareTransaction(txnToSend, { wallet: wal }).then(txn => {
            let ser = rbc.encode(txn);
            fee(ser).then(fees => {
                let baseDrops = fees.base_fee

                delete txnToSend['SigningPubKey']
                txnToSend['Fee'] = baseDrops + '';


                api.prepareTransaction(txnToSend, { wallet: wal }).then(txn => {
                    resolve(txn);
                }).catch(e => { reject(e); });
            }).catch(e => { reject(e); });
        }).catch(e => { reject(e); });
    });
}

const feeSubmit = (seed, txn) => {
    return new Promise((resolve, reject) => {
        feeCompute(seed, txn).then(txn => {
            api.submit(txn,
                { wallet: xrpljs.Wallet.fromSeed(seed) }).then(s => {
                    resolve(s);
                }).catch(e => { reject(e); });
        }).catch(e => { reject(e); });
    });
}

const submitTxn = (seed, txn) => {
    return new Promise((resolve, reject) => {
        api.connect().then(() => {
            feeSubmit(seed, txn).then(res => {
                if (res?.result?.engine_result === 'tesSUCCESS')
                    resolve(res?.result?.engine_result);
                else
                    reject(res);
            }).catch(e => { reject(e); });
        }).catch(e => { reject(e); });
    });
}

const setHook = async (secret) => {
    const account = xrpljs.Wallet.fromSeed(secret)
    const binary = fs.readFileSync(WASM_PATH).toString('hex').toUpperCase();
    const hookTx = {
        Account: account.classicAddress,
        TransactionType: "SetHook",
        Hooks:
            [
                {
                    Hook: {
                        CreateCode: binary.slice(0, 194252),
                        HookOn: '0000000000000000',
                        HookNamespace: NAMESPACE,
                        HookApiVersion: 0,
                        Flags: hsfOVERRIDE
                    }
                },
                { Hook: { Flags: hsfOVERRIDE || hsfNSDELETE, CreateCode: '' } },
                { Hook: { Flags: hsfOVERRIDE || hsfNSDELETE, CreateCode: '' } },
                { Hook: { Flags: hsfOVERRIDE || hsfNSDELETE, CreateCode: '' } }
            ]
    };

    return await submitTxn(secret, hookTx);
}

const main = async () => {
    if (mode == ALL || mode == MIGRATE)
        await setHook(governorSecret).then(console.log);
    if (mode == ALL || mode == CLEAR)
        await setHook(registrySecret).then(console.log);
}

main().catch(console.error).finally(() => process.exit(0));