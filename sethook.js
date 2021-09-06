const RippleAPI = require('ripple-lib').RippleAPI;
const keypairs = require('ripple-keypairs');
const fs = require('fs');
const api = new RippleAPI({ server: 'wss://hooks-testnet.xrpl-labs.com'});
// Hook account
// { "address": "rLj1Ea4eeTdgAGoAXSCEk8dvqHewFeoeb7", "secret": "sh4HxDesptxE7aeE8LzRBTM3EooEP", "xrp": 10000, "hash": "78DEA38A0E35BE878DF55DD698C8700D690C7C2168262CB84B6ECF7F86A8AAEA", "code": "tesSUCCESS" }

// Host account
// { "address": "rG7CmxCPDa9wBwDVn7vBetSBpqQ3rWGtfi", "secret": "spikf8wK34zeGHQSaJ4AiDTttgonM", "xrp": 10000, "hash": "2737215204809E1C822A12182AAAF433AF86740F25526E0A8AB8C4E04CA6A1B0", "code": "tesSUCCESS" }

const wasmfile = process.argv[2] || "build/evernode.wasm";
const secret = process.argv[3] || "sh4HxDesptxE7aeE8LzRBTM3EooEP";
const address = keypairs.deriveAddress(keypairs.deriveKeypair(secret).publicKey)

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
        console.log(x.txJSON);
        let s = api.sign(x.txJSON, secret)
        console.log(s)
        api.submit(s.signedTransaction).then(response => {
            console.log(response.resultCode, response.resultMessage);
            process.exit(0);
        }).catch(e => { console.log(e) });
    });
}).then(() => { }).catch(console.error);