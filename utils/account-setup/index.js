const evernode = require('evernode-js-client');
const https = require('https'); 

const { XrplAccount, XrplApi } = evernode;


const TOTAL_MINTED_EVRS = "72253440";
const FOUNDATION_EVRS = "51609600";
//const AIR_DROP_AMOUNT = TOTAL_MINTED_EVRS - FOUNDATION_EVRS;

// Testnets --------------------------------------------------------------
const FAUCETS_URL = 'https://faucet-nft.ripple.com/accounts';
const rippledServer = 'wss://xls20-sandbox.rippletest.net:51233';

// Account names 
const accounts = ["ISSUER", "FOUNDATION", "COMMUNITY_BANK"];

function httpPost(url) {
    return new Promise((resolve, reject) => {
        const req = https.request(url, { method: 'POST' }, (resp) => {
            let data = '';
            resp.on('data', (chunk) => data += chunk);
            resp.on('end', () => {
                if (resp.statusCode == 200)
                    resolve(data);
                else
                    reject(data);
            });
        })

        req.on("error", reject);
        req.on('timeout', () => reject('Request timed out.'))
        req.end()
    })
}

function sleep(milliseconds) {
    const date = Date.now();
    let currentDate = null;
    do {
      currentDate = Date.now();
    } while (currentDate - date < milliseconds);
  }


async function main () {
    try {
        const xrplApi = new XrplApi(rippledServer);
        await xrplApi.connect();
        
        console.log('Started to create XRP Accounts');
        const unresolved = accounts.map(async function(item) {
            const resp = await httpPost(FAUCETS_URL);           
            const json = JSON.parse(resp);
            console.log(json);
            const acc = new XrplAccount(json.account.address, json.account.secret, { xrplApi: xrplApi });
            console.log(`Created ${item} Account` );

            return { "name" : item, "xrplAcc": acc };
            
          });

        const newAccounts = await Promise.all(unresolved);
        console.log(newAccounts); 

        sleep(5000);
        
        // Initialte Trust Lines and Currencies trnasfers
        const lines_issuer_to_foundation = await newAccounts[0].xrplAcc.getTrustLines(evernode.EvernodeConstants.EVR, newAccounts[1].xrplAcc.address);

        if (lines_issuer_to_foundation.length === 0) {
            await newAccounts[0].xrplAcc.setTrustLine(evernode.EvernodeConstants.EVR, newAccounts[1].xrplAcc.address, TOTAL_MINTED_EVRS);
        }

        const lines_foundation_to_comm_bank = await newAccounts[1].xrplAcc.getTrustLines(evernode.EvernodeConstants.EVR, newAccounts[2].xrplAcc.address);

        if (lines_foundation_to_comm_bank.length === 0) {
            await newAccounts[1].xrplAcc.setTrustLine(evernode.EvernodeConstants.EVR, newAccounts[2].xrplAcc.address, FOUNDATION_EVRS);
        }
        
    } catch (err) {
        console.log(err);
        console.log("Evernode account setup exiting with an error.");
        process.exit(1);
    }

}

main().catch(console.error);





