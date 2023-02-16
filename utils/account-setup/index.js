const https = require('https');
const process = require('process');
const fs = require('fs');
const path = require('path');
const { XrplAccount, XrplApi, EvernodeConstants, Defaults } = require('evernode-js-client');

const TOTAL_MINTED_EVRS = "72253440";
const REWARD_EVRS = "51609600";
const CONFIG_FILE = 'accounts.json';

// BEGIN - Endpoints.

/**
 * NOTE: According to the environment that you are going to work in, the response payload structure of the APIs might be changed.
 * Hence, you have to change the payload handling points accordingly.
*/

// NFT DEV NET URLs.
// const FAUCETS_URL = process.env.CONF_FAUCETS_URL || 'https://faucet-nft.ripple.com/accounts';
// const RIPPLED_URL = process.env.CONF_RIPPLED_URL || 'wss://xls20-sandbox.rippletest.net:51233';

// Hooks V2 TEST NET URLs.
const FAUCETS_URL = process.env.CONF_FAUCETS_URL || 'https://hooks-testnet-v3.xrpl-labs.com/newcreds';
const RIPPLED_URL = process.env.CONF_RIPPLED_URL || 'wss://hooks-testnet-v3.xrpl-labs.com';
const NETWORK_ID = 21338;

// END - Endpoints.

const ACCOUNT_DATA_DIR = process.env.ACCOUNT_DATA_DIR || __dirname;
const HOOK_DATA_DIR = ACCOUNT_DATA_DIR + '/data'

// Account names
const accounts = ["ISSUER", "FOUNDATION_COLD_WALLET", "GOVERNOR", "REGISTRY", "HEARTBEAT"];

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


async function main() {

    // BEGIN - Connect to XRPL API
    const xrplApi = new XrplApi(RIPPLED_URL);
    Defaults.set({
        xrplApi: xrplApi,
        networkID: NETWORK_ID
    })
    await xrplApi.connect();
    // END - Connect to XRPL API

    try {

        // BEGIN - Account Creation 
        console.log('Started to create XRP Accounts');
        const newAccounts = [];

        for (const account of accounts) {
            const resp = await httpPost(FAUCETS_URL);
            const json = JSON.parse(resp);

            // If Hooks TEST NET is used.
            const acc = new XrplAccount(json.address, json.secret, { xrplApi: xrplApi });

            // If NFT DEV NET is used.
            // const acc = new XrplAccount(json.account.address, json.account.secret, { xrplApi: xrplApi });

            if (account === "ISSUER") {
                await new Promise(r => setTimeout(r, 5000));
                await acc.setAccountFields({ Flags: { asfDefaultRipple: true } });

                console.log('Enabled Rippling in ISSUER Account');
            }

            console.log(`Created ${account} Account`);

            newAccounts.push({ "name": account, "xrplAcc": acc });

            // Keep 10 seconds gap between two API calls.
            await new Promise(r => setTimeout(r, 10000));

        }

        // END - Account Creation

        // 5 second wait in order to sync the accounts
        await new Promise(r => setTimeout(r, 5000));

        // BEGIN - Trust Lines initiation
        const foundation_lines = await newAccounts[1].xrplAcc.getTrustLines(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        if (foundation_lines.length === 0) {
            await newAccounts[1].xrplAcc.setTrustLine(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, TOTAL_MINTED_EVRS);
        }

        const governor_lines = await newAccounts[2].xrplAcc.getTrustLines(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        if (governor_lines.length === 0) {
            await newAccounts[2].xrplAcc.setTrustLine(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, TOTAL_MINTED_EVRS);
        }

        const registry_lines = await newAccounts[3].xrplAcc.getTrustLines(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        if (registry_lines.length === 0) {
            await newAccounts[3].xrplAcc.setTrustLine(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, TOTAL_MINTED_EVRS);
        }

        const heartbeat_hook_lines = await newAccounts[4].xrplAcc.getTrustLines(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        if (heartbeat_hook_lines.length === 0) {
            await newAccounts[4].xrplAcc.setTrustLine(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, TOTAL_MINTED_EVRS);
        }

        console.log("Trust Lines initiated");
        // END - Trust Lines initiation

        // BEGIN - Transfer Currency
        await newAccounts[0].xrplAcc.makePayment(newAccounts[1].xrplAcc.address, TOTAL_MINTED_EVRS, EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        console.log(`${TOTAL_MINTED_EVRS} EVRs were issued to EVERNODE Foundation`);

        await newAccounts[1].xrplAcc.makePayment(newAccounts[4].xrplAcc.address, REWARD_EVRS, EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        console.log(`${REWARD_EVRS} EVRs were transferred to Heartbeat by the Foundation`);
        // END - Transfer Currency

        // ISSUER Blackholing
        await newAccounts[0].xrplAcc.setRegularKey(BLACKHOLE_ADDRESS);
        await newAccounts[0].xrplAcc.setAccountFields({ Flags: { asfDisableMaster: true } });

        console.log("Blackholed ISSUER");

        // BEGIN - Log Account Details
        console.log('\nAccount Details -------------------------------------------------------');

        let config = {};
        newAccounts.forEach(element => {
            // Convert snake_case to camelCase.
            const configKey = element.name.toLowerCase().replace(/_([a-z])/g, function (g) { return g[1].toUpperCase(); });
            config[configKey] = {
                address: element.xrplAcc.address
            };
            console.log(`Account name :${element.name}`);
            console.log(`Address : ${element.xrplAcc.address}`);
            if (element.name !== "ISSUER") {
                console.log(`Secret : ${element.xrplAcc.secret}`);
                config[configKey].secret = element.xrplAcc.secret;
            }
            console.log('-----------------------------------------------------------------------');

        });
        // END - Log Account Details

        // Save the generated account data in the config.
        const configDir = path.resolve(HOOK_DATA_DIR, config.governor.address);
        fs.mkdirSync(configDir, { recursive: true });
        const configPath = `${configDir}/${CONFIG_FILE}`;
        console.log(`Recording account data in ${configPath}`);
        fs.writeFileSync(configPath, JSON.stringify(config, null, 4));

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