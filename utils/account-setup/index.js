const https = require('https');
const process = require('process');
const codec = require('ripple-address-codec');
const { Buffer } = require('buffer');
const { XrplAccount, XrplApi, EvernodeConstants } = require('evernode-js-client');
const { exec } = require("child_process");

const TOTAL_MINTED_EVRS = "72253440";
const PURCHASER_COLD_WALLET_EVRS = "51609600";
const MIN_XRP = "1";
const INIT_MEMO_TYPE = "evnInitialize"; // This is kept only here as a constant, since we don't want to expose this event to public.
const INIT_MEMO_FORMAT = "hex";

// End Points --------------------------------------------------------------
const FAUCETS_URL = process.env.CONF_FAUCETS_URL || 'https://faucet-nft.ripple.com/accounts';
const RIPPLED_URL = process.env.CONF_RIPPLED_URL || 'wss://xls20-sandbox.rippletest.net:51233';
const MODE = process.env.MODE || 'dev';

const DATA_DIR = process.env.DATA_DIR || __dirname;
const BIN_DIR = process.env.BIN_DIR || __dirname;

const HOOK_EMULATOR_PATH = BIN_DIR + ((!process.env.BIN_DIR && !BIN_DIR.endsWith('/dist')) ? '/dist' : '') + '/hook-emulator';
const HOOK_EMULATOR_DATA_DIR = DATA_DIR + ((!process.env.DATA_DIR && !DATA_DIR.endsWith('/dist')) ? '/dist' : '') + '/hook-emulator';

// Account names 
const accounts = ["ISSUER", "FOUNDATION_COLD_WALLET", "PURCHASER_COLD_WALLET", "REGISTRY", "PURCHASER_HOT_WALLET"];

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

async function executeEmulator(registryAddress, registrySecret, configInitHandler) {
    // Run the emulator process with registry account info.
    const emulatorProc = exec(`RIPPLED_URL=${RIPPLED_URL} MODE=${MODE} FILE_LOG=1  DATA_DIR=${HOOK_EMULATOR_DATA_DIR} \
    BIN_DIR=${HOOK_EMULATOR_PATH} $(which node) ${HOOK_EMULATOR_PATH} ${registryAddress} ${registrySecret}`);
    // Pipe child stdout and stderr to the parent process.
    emulatorProc.stdout.pipe(process.stdout);
    emulatorProc.stderr.pipe(process.stderr);
    // Handle config initialization if registry contract listen message is logged in child emulator.
    emulatorProc.stdout.on('data', async (data) => {
        if (data.includes(`Listening to hook address ${registryAddress}`))
            await configInitHandler();
    });

    // Complete the promise on child process exit.
    return new Promise((resolve, reject) => {
        emulatorProc.on('exit', async (code) => code >= 0 ? resolve(code) : reject(code));
    });
}


async function main() {

    // BEGIN - Connect to XRPL API 
    const xrplApi = new XrplApi(RIPPLED_URL);
    await xrplApi.connect();
    // END - Connect to XRPL API

    try {

        // BEGIN - Account Creation 
        console.log('Started to create XRP Accounts');
        const unresolved = accounts.map(async function (item) {
            const resp = await httpPost(FAUCETS_URL);
            const json = JSON.parse(resp);
            const acc = new XrplAccount(json.account.address, json.account.secret, { xrplApi: xrplApi });

            if (item === "ISSUER") {
                await new Promise(r => setTimeout(r, 5000));
                await acc.setAccountFields({ Flags: { asfDefaultRipple: true } });

                console.log('Enabled Rippling in ISSUER Account');
            }

            console.log(`Created ${item} Account`);

            return { "name": item, "xrplAcc": acc };

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
            await newAccounts[2].xrplAcc.setTrustLine(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, PURCHASER_COLD_WALLET_EVRS);
        }

        const registry_lines = await newAccounts[3].xrplAcc.getTrustLines(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        if (registry_lines.length === 0) {
            await newAccounts[3].xrplAcc.setTrustLine(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, TOTAL_MINTED_EVRS);
        }

        const purchaser_lines = await newAccounts[4].xrplAcc.getTrustLines(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        if (purchaser_lines.length === 0) {
            await newAccounts[4].xrplAcc.setTrustLine(EvernodeConstants.EVR, newAccounts[0].xrplAcc.address, PURCHASER_COLD_WALLET_EVRS);
        }

        console.log("Trust Lines initiated");
        // END - Trust Lines initiation

        // BEGIN - Transfer Currency
        await newAccounts[0].xrplAcc.makePayment(newAccounts[1].xrplAcc.address, TOTAL_MINTED_EVRS, EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        console.log(`${TOTAL_MINTED_EVRS} EVRs were issued to EVERNODE Foundation`);

        await newAccounts[1].xrplAcc.makePayment(newAccounts[2].xrplAcc.address, PURCHASER_COLD_WALLET_EVRS, EvernodeConstants.EVR, newAccounts[0].xrplAcc.address);

        console.log(`${PURCHASER_COLD_WALLET_EVRS} EVRs were transferred to Purchaser cold wallet by the Foundation`);
        // END - Transfer Currency

        // ISSUER Blackholing	
        await newAccounts[0].xrplAcc.setRegularKey(BLACKHOLE_ADDRESS);
        await newAccounts[0].xrplAcc.setAccountFields({ Flags: { asfDisableMaster: true } });

        console.log("Blackholed ISSUER");

        // BEGIN - Log Account Details
        console.log('\nAccount Details -------------------------------------------------------');

        newAccounts.forEach(element => {
            console.log(`Account name :${element.name}`);
            console.log(`Address : ${element.xrplAcc.address}`);
            if (element.name !== "ISSUER") {
                console.log(`Secret : ${element.xrplAcc.secret}`);
            }
            console.log('-----------------------------------------------------------------------');

        });
        // END - Log Account Details

        // Execute the emulator.
        console.log('Executing the registry hook emulator.');
        await executeEmulator(newAccounts[3].xrplAcc.address, newAccounts[3].xrplAcc.secret, async () => {
            // Send the init transaction with min xrp to the registry contract from foundation account.
            console.log('Sending intialize transaction to the registry account.');

            // Get issuer and foundation cold wallet account ids.
            let memoData = Buffer.allocUnsafe(40);
            codec.decodeAccountID(newAccounts[0].xrplAcc.address).copy(memoData);
            codec.decodeAccountID(newAccounts[1].xrplAcc.address).copy(memoData, 20);
            await newAccounts[1].xrplAcc.makePayment(newAccounts[3].xrplAcc.address, MIN_XRP, 'XRP', null,
                [{ type: INIT_MEMO_TYPE, format: INIT_MEMO_FORMAT, data: memoData.toString('hex') }]);

            // Once the initialize transaction is done, we don't need xrpl api connection for the account setup tool anymore.
            // So we disconnect it.
            await xrplApi.disconnect();
        }).catch(e => { throw `Hook emulator execution faild with exit code ${e}` });

        console.log('Hook emulator execution exited.');

    } catch (err) {
        console.log(err);
        console.log("Evernode account setup exiting with an error.");
        // We have to do the disconnect only in catch block because it'll be disconnected after config initialization in success flow.
        await xrplApi.disconnect();
        process.exit(1);
    }
    finally {
        process.exit();
    }
}

main().catch(console.error);