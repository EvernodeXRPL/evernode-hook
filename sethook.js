const RippleAPI = require('ripple-lib').RippleAPI;
const fs = require('fs');
const api = new RippleAPI({ server: 'wss://hooks-testnet.xrpl-labs.com' });

const DEFAULT_ADDR = 'rH7ZRrh4sFuwei9gGz6kgrJUmviimYnz2q';
const DEFAULT_SECRET = 'sntFtyfH3J4dpbov7RhxZvdWZwzJv';

// Hook account
// { "address": "rB9gfgPcZ9aS1WF2D1MPS8Gdg8bs2y8aEK", "secret": "snL3hEFU3JUT7WSFRHvyKmghjU1Nj", "xrp": 10000, "hash": "78DEA38A0E35BE878DF55DD698C8700D690C7C2168262CB84B6ECF7F86A8AAEA", "code": "tesSUCCESS" }

// Host account
// { "address": "r9D6VdU5odLPaR9aWYMhCXV1wztB9spBqR", "secret": "sh18veYyyfz3JHNp61js8eKPvZHMi", "xrp": 10000, "hash": "2737215204809E1C822A12182AAAF433AF86740F25526E0A8AB8C4E04CA6A1B0", "code": "tesSUCCESS" }

const cfgPath = 'hook.cfg';
let cfg;

if (!fs.existsSync(cfgPath)) {
    cfg = {
        address: DEFAULT_ADDR,
        secret: DEFAULT_SECRET
    }
    fs.writeFileSync(cfgPath, JSON.stringify(cfg, null, 2));
}
else {
    cfg = JSON.parse(fs.readFileSync(cfgPath));
}



const wasmfile = process.argv[2] || "build/evernode.wasm";
const secret = cfg.secret;
const address = api.deriveAddress(api.deriveKeypair(secret).publicKey);

console.log("SetHook on address " + address);

api.on('error', (errorCode, errorMessage) => { console.log(errorCode + ': ' + errorMessage); });
api.on('connected', () => { console.log('connected'); });
api.on('disconnected', (code) => { console.log('disconnected, code:', code); });
api.connect().then(() => {
    binary = fs.readFileSync(wasmfile).toString('hex').toUpperCase();
    j = {
        Account: address,
        TransactionType: "SetHook",
        CreateCode: binary,
        HookOn: '0000000000000000'
    }
    api.prepareTransaction(j).then((x) => {
        let s = api.sign(x.txJSON, secret)
        api.submit(s.signedTransaction).then(response => {
            console.log(response.resultCode, response.resultMessage);
            process.exit(0);
        }).catch(e => { console.log(e) });
    });
}).then(() => { }).catch(console.error);