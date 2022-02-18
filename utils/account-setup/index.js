const evernode = require('evernode-js-client');
const https = require('https'); 

const { XrplAccount, XrplApi } = evernode;


const TOTAL_MINTED_EVRS = "72253440";
const PURCHASER_PROGRAM_EVRS = "51609600";
//const AIR_DROP_AMOUNT = TOTAL_MINTED_EVRS - FOUNDATION_EVRS;

// Testnets --------------------------------------------------------------
const FAUCETS_URL = 'https://faucet-nft.ripple.com/accounts';
const rippledServer = 'wss://xls20-sandbox.rippletest.net:51233';

// Account names 
const accounts = ["ISSUER", "FOUNDATION", "COMMUNITY_BANK", "REGISTRY"];

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

// TODO : Need to revisit
function sleep(milliseconds) {
    const date = Date.now();
    let currentDate = null;
    do {
      currentDate = Date.now();
    } while (currentDate - date < milliseconds);
}

async function main () {
    try {

        // BEGIN - Connect to XRPL API 
        const xrplApi = new XrplApi(rippledServer);
        await xrplApi.connect();
        // END - Connect to XRPL API
        

        // BEGIN - Account Creation 
        console.log('Started to create XRP Accounts');
        const unresolved = accounts.map(async function(item) {
            const resp = await httpPost(FAUCETS_URL);           
            const json = JSON.parse(resp);
            const acc = new XrplAccount(json.account.address, json.account.secret, { xrplApi: xrplApi });

            if (item === "ISSUER") {
                await acc.setDefaultRippling(true);
                console.log('Enabled Rippling in ISSUER Account');
            }

            console.log(`Created ${item} Account`);

            return { "name" : item, "xrplAcc": acc };
            
          });

        const newAccounts = await Promise.all(unresolved);
        // END - Account Creation

        // 5 millisecond wait in order to sync the accounts
        sleep(5000);
        
        // BEGIN - Trust Lines initiation
        const foundation_lines = await newAccounts[1].xrplAcc.getTrustLines(evernode.EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        if (foundation_lines.length === 0) {
            await newAccounts[1].xrplAcc.setTrustLine(evernode.EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, TOTAL_MINTED_EVRS);
        }

        const comm_bank_lines = await newAccounts[2].xrplAcc.getTrustLines(evernode.EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        if (comm_bank_lines.length === 0) {
            await newAccounts[2].xrplAcc.setTrustLine(evernode.EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, PURCHASER_PROGRAM_EVRS);
        }

        const registry_lines = await newAccounts[3].xrplAcc.getTrustLines(evernode.EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        if (registry_lines.length === 0) {
            await newAccounts[3].xrplAcc.setTrustLine(evernode.EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, TOTAL_MINTED_EVRS);
        }

        console.log("Trust Lines initiated");
        // END - Trust Lines initiation

        // BEGIN - Transfer Currency
        await newAccounts[0].xrplAcc.makePayment(newAccounts[1].xrplAcc.address, TOTAL_MINTED_EVRS, evernode.EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        console.log(`${TOTAL_MINTED_EVRS}/- EVR amount was issued to EVERNODE Foundation`);

        await newAccounts[1].xrplAcc.makePayment(newAccounts[2].xrplAcc.address, PURCHASER_PROGRAM_EVRS, evernode.EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        console.log(`${PURCHASER_PROGRAM_EVRS}/- EVR amount was transfered to Community Bank Account"`);
        // END - Transfer Currency

        // BEGIN - Log Account Details
        console.log('\nAccount Details -------------------------------------------------------');

        newAccounts.forEach(element => {            
            console.log(`Account name :${element.name}`);
            console.log(`Address : ${element.xrplAcc.address}`);
            if (element.name  !== "ISSUER") {
                console.log(`Secret : ${element.xrplAcc.secret}`);
            }

            console.log('-----------------------------------------------------------------------');
            
        });
        // END - Log Account Details

        process.exit();
      
    } catch (err) {
        console.log(err);
        console.log("Evernode account setup exiting with an error.");
        process.exit(1);
    }

}

main().catch(console.error);