const fs = require('fs');
const process = require('process');
const xrpljs = require('xrpl-hooks');
const rbc = require('xrpl-binary-codec');

const hsfOVERRIDE = 1;
// const hsfNSDELETE = 2;
// const hfsOVERRIDE = 1;
// const hfsNSDELETE = 2;

// sha256('evernode.org|registry')
const NAMESPACE = '01EAF09326B4911554384121FF56FA8FECC215FDDE2EC35D9E59F2C53EC665A0'

const server = 'wss://hooks-testnet-v2.xrpl-labs.com';

const CONFIG_PATH = process.env.CONFIG_PATH || 'hook.json';
const WASM_PATH = process.env.WASM_PATH || "build/evernode.wasm"

let cfg;

if (!fs.existsSync(CONFIG_PATH)) {
    cfg = {
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

const secret = cfg.registry.secret;

if (!secret) {
    console.error("SETHOOK FAILED: Please specify hook secret in hook.json");
    process.exit(1);
}
else {
    const api = new xrpljs.Client(server);

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
                }
            ]
    };

    submitTxn(secret, hookTx).then(res => { console.log(res); }).catch(console.error).finally(() => process.exit(0))
}