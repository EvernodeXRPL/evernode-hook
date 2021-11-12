const RippleAPI = require('ripple-lib').RippleAPI;
const fs = require('fs');
const api = new RippleAPI({ server: 'wss://hooks-testnet.xrpl-labs.com' });

const cfgPath = 'hook.cfg';
let cfg;

if (!fs.existsSync(cfgPath)) {
    cfg = {
        secret: undefined
    }
    fs.writeFileSync(cfgPath, JSON.stringify(cfg, null, 2));
}
else {
    cfg = JSON.parse(fs.readFileSync(cfgPath));
}

if (cfg.secret === undefined) {
    console.log("SETHOOK FAILED: Please specify hook secret in hook.cfg");
}
else {
    const wasmfile = process.argv[2] || "build/evernode.wasm";
    const secret = cfg.secret;
    const address = api.deriveAddress(api.deriveKeypair(secret).publicKey);

    console.log("SetHook on address: " + address);

    api.on('error', (errorCode, errorMessage) => { console.log(errorCode + ': ' + errorMessage); });
    api.on('connected', () => { console.log('Connected'); });
    api.on('disconnected', (code) => { console.log('Disconnected, code: ', code); });
    api.connect().then(() => {
        binary = fs.readFileSync(wasmfile).toString('hex').toUpperCase();
        tx = {
            Account: address,
            TransactionType: "SetHook",
            CreateCode: binary,
            HookOn: '0000000000000000'
        }
        api.prepareTransaction(tx).then((x) => {
            let signed_tx = api.sign(x.txJSON, secret)
            api.submit(signed_tx.signedTransaction).then(response => {
                console.log(response.resultCode, response.resultMessage);
                process.exit(0);
            }).catch(e => { console.log(e) });
        });
    }).then(() => { }).catch(console.log('Error: ' + console.error);
}
