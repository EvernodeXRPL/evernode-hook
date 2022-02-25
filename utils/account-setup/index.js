const https = require('https'); 
const process = require('process'); 
const { XrplAccount, XrplApi, EvernodeConstants } = require('evernode-js-client');

const TOTAL_MINTED_EVRS = "72253440";
const PURCHASER_PROGRAM_EVRS = "51609600";

// End Points --------------------------------------------------------------
const FAUCETS_URL = process.env.CONF_FAUCETS_URL || 'https://faucet-nft.ripple.com/accounts';
const RIPPLED_URL = process.env.CONF_RIPPLED_URL || 'wss://xls20-sandbox.rippletest.net:51233';

// Account names 
const accounts = ["ISSUER", "FOUNDATION", "COMMUNITY_CONTRACT_BANK", "REGISTRY", "PURCHASER"];

// XRP Pre-defined Special Address -> Blackhole
const BLACKHOLE_ADDRESS = "rrrrrrrrrrrrrrrrrrrn5RM1rHd";

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


async function main () {

    // BEGIN - Connect to XRPL API 
    const xrplApi = new XrplApi(RIPPLED_URL);
    await xrplApi.connect();
    // END - Connect to XRPL API

    try {       

        // BEGIN - Account Creation 
        console.log('Started to create XRP Accounts');
        const unresolved = accounts.map(async function(item) {
            const resp = await httpPost(FAUCETS_URL);           
            const json = JSON.parse(resp);
            const acc = new XrplAccount(json.account.address, json.account.secret, { xrplApi: xrplApi });

            if (item === "ISSUER") {
                await new Promise(r => setTimeout(r, 5000));
                await acc.setAccountFields({ Flags: { asfDefaultRipple : true} });

                console.log('Enabled Rippling in ISSUER Account');
            }

            console.log(`Created ${item} Account`);

            return { "name" : item, "xrplAcc": acc };
            
          });

        const newAccounts = await Promise.all(unresolved);
        // END - Account Creation

        // 5 second wait in order to sync the accounts
        await new Promise(r => setTimeout(r, 5000));
        
        // BEGIN - Trust Lines initiation
        const foundation_lines = await newAccounts[1].xrplAcc.getTrustLines(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        if (foundation_lines.length === 0) {
            await newAccounts[1].xrplAcc.setTrustLine(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, TOTAL_MINTED_EVRS);
        }

        const comm_bank_lines = await newAccounts[2].xrplAcc.getTrustLines(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        if (comm_bank_lines.length === 0) {
            await newAccounts[2].xrplAcc.setTrustLine(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, PURCHASER_PROGRAM_EVRS);
        }

        const registry_lines = await newAccounts[3].xrplAcc.getTrustLines(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        if (registry_lines.length === 0) {
            await newAccounts[3].xrplAcc.setTrustLine(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, TOTAL_MINTED_EVRS);
        }

        const purchaser_lines = await newAccounts[4].xrplAcc.getTrustLines(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        if (purchaser_lines.length === 0) {
            await newAccounts[4].xrplAcc.setTrustLine(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, PURCHASER_PROGRAM_EVRS);
        }

        console.log("Trust Lines initiated");
        // END - Trust Lines initiation

        // BEGIN - Transfer Currency
        await newAccounts[0].xrplAcc.makePayment(newAccounts[1].xrplAcc.address, TOTAL_MINTED_EVRS, EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        console.log(`${TOTAL_MINTED_EVRS} EVRs were issued to EVERNODE Foundation`);

        await newAccounts[1].xrplAcc.makePayment(newAccounts[2].xrplAcc.address, PURCHASER_PROGRAM_EVRS, EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        console.log(`${PURCHASER_PROGRAM_EVRS} EVRs were transferred to Community Contract Bank Account by the Foundation`);
        // END - Transfer Currency

        // ISSUER Blackholing	
        await newAccounts[0].xrplAcc.setRegularKey(BLACKHOLE_ADDRESS);
        await newAccounts[0].xrplAcc.setAccountFields({ Flags: { asfDisableMaster : true} });

        console.log("Blackholed ISSUER");

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
      
    } catch (err) {
        console.log(err);
        console.log("Evernode account setup exiting with an error.");
        process.exit(1);
    }
    finally {
        await xrplApi.disconnect();
        process.exit();
    }
}

main().catch(console.error);