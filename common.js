const xrpljs = require('xrpl-hooks');
const rbc = require('xrpl-binary-codec');

const appenv = {
    hsfOVERRIDE: 1,
    hsfNSDELETE: 2,
    hfsOVERRIDE: 1,
    hfsNSDELETE: 2,
    NAMESPACE: '01EAF09326B4911554384121FF56FA8FECC215FDDE2EC35D9E59F2C53EC665A0', // sha256('evernode.org|registry')
    SERVER: 'wss://hooks-testnet-v2.xrpl-labs.com',
    CONFIG_PATH: process.env.CONFIG_PATH || 'hook.json',
    WASM_DIR_PATH: process.env.WASM_PATH || "build",
    PARAM_STATE_HOOK_KEY: '4556520100000000000000000000000000000000000000000000000000000001'
}

const api = new xrpljs.Client(appenv.SERVER);

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

const getHookHashes = (account) => {
    return new Promise((resolve, reject) => {
        api.connect().then(() => {
            let req = { command: 'account_objects' };
            if (account)
                req['account'] = account;

            api.request(req).then(resp => {
                resolve(resp.result.account_objects.find(o => o.Hooks && o.Hooks.length)?.Hooks?.map(h => h?.Hook?.HookHash));
            }).catch(e => {
                reject(e);
            });
        });
    });
};

module.exports = {
    submitTxn,
    getHookHashes,
    appenv
}