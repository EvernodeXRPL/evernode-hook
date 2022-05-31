const fs = require('fs')
const xrpljs = require('xrpl-hooks');
const rbc = require('xrpl-binary-codec');

const hsfOVERRIDE = 1;
const hsfNSDELETE = 2;
const hfsOVERRIDE = 1;
const hfsNSDELETE = 2;

const server = 'wss://hooks-testnet-v2.xrpl-labs.com';

const cfgPath = 'hook.cfg';
let cfg;

if (!fs.existsSync(cfgPath)) {
    cfg = {
        secret: ""
    }
    fs.writeFileSync(cfgPath, JSON.stringify(cfg, null, 2));
}
else {
    cfg = JSON.parse(fs.readFileSync(cfgPath));
}

if (cfg.secret === "") {
    console.log("SETHOOK FAILED: Please specify hook secret in hook.cfg");
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
            txnToSend = { ...txnOrg };
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

    const account = xrpljs.Wallet.fromSeed(cfg.secret)
    const binary = fs.readFileSync("build/evernode.wasm").toString('hex').toUpperCase();
    const hookTx = {
        Account: account.classicAddress,
        TransactionType: "SetHook",
        Hooks:
            [
                {
                    Hook: {
                        CreateCode: binary.slice(0, 194252),
                        HookOn: '0000000000000000',
                        HookNamespace: 'CAFECAFECAFECAFECAFECAFECAFECAFECAFECAFECAFECAFECAFECAFECAFECAFE',
                        HookApiVersion: 0,
                        Flags: hsfOVERRIDE
                    }
                }
            ]
    };

    submitTxn(cfg.secret, hookTx).then(res => { console.log(res); }).catch(console.error).finally(() => process.exit(0))
}