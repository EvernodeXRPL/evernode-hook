const { XrplAccount, XrplApi, Defaults, EvernodeConstants } = require('evernode-js-client');
const fs = require('fs')
const xrpljs = require('xrpl-hooks');
const rbc = require('xrpl-binary-codec');


const server = 'ws://localhost:6005';

const genesisAddress = "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh";
const genesisSecret = "snoPBrXtMeMyMHUVTgbuqAfg1SUTb";

const TOTAL_MINTED_EVRS = "72253440";
// XRP Pre-defined Special Address -> Blackhole
const BLACKHOLE_ADDRESS = "rrrrrrrrrrrrrrrrrrrn5RM1rHd";
const hsfOVERRIDE = 1;
const hsfNSDELETE = 2;
const hfsOVERRIDE = 1;
const hfsNSDELETE = 2;
const NAMESPACE = '01EAF09326B4911554384121FF56FA8FECC215FDDE2EC35D9E59F2C53EC665A0'



const accounts = {
    issuer: {
        address: "rP9bHYzzH91wW51L4VMCZgMUxWfMxzMyBK",
        secret: "sskV63uw6YmE8YsmCmnBMDzmNcRsJ"
    },
    foundation: {
        address: "rwEVcdR7HMdJtCpcwHFHWe4AnzaPjAaph4",
        secret: "ssma2d2ySK1RbtPoGhNC48KamFRaY"
    },
    registry: {
        address: "rBtpWXY7e7GHYR4YceF4dG4jcUYiemFzci",
        secret: "shrvS4mJAFFeiZmhkrGGRmwQaaLmR"
    },
    host: {
        address: "r3PKGU5Uzbck3nSXzjoUiK4wiJT7eCwUxC",
        secret: "snPyvMBLESA3hoAb6RCpwDEMDkrUr"
    },
    tenant: {
        address: "reL4CVVPZFSkr6SXdHRxjEXDt5crhwbcE",
        secret: "snBKhh1hkpNaWLLNmtqk1QZs92nZh"
    }
}

async function main() {
    const xrplApi = new XrplApi(server);
    try {
        await xrplApi.connect();
        console.log('Connected to server');
        Defaults.set({
            xrplApi: xrplApi
        });

        const genesisAcc = new XrplAccount(genesisAddress, genesisSecret);

        const newAccounts = [];
        // Creating required accounts on genesis ledger.
        for (let key in accounts) {
            const account = accounts[key];
            const acc = new XrplAccount(account.address, account.secret);
            console.log(`Transfering funds to ${key} account.`);
            await genesisAcc.makePayment(account.address, '10000000000', 'XRP');
            if (key === 'issuer') {
                await acc.setAccountFields({ Flags: { asfDefaultRipple: true } });
            }
            newAccounts.push({
                name: key,
                xrplAcc: acc
            });
            newAccounts[key] = acc;
        }

        // BEGIN - Trust Lines initiation
        const foundation_lines = await newAccounts['foundation'].getTrustLines(EvernodeConstants.EVR, newAccounts['issuer'].address);

        if (foundation_lines.length === 0) {
            await newAccounts['foundation'].setTrustLine(EvernodeConstants.EVR, newAccounts['issuer'].address, TOTAL_MINTED_EVRS);
            console.log("Created foundation -> issuer trust line.");
        }

        const registry_lines = await newAccounts['registry'].getTrustLines(EvernodeConstants.EVR, newAccounts['issuer'].address);

        if (registry_lines.length === 0) {
            await newAccounts['registry'].setTrustLine(EvernodeConstants.EVR, newAccounts['issuer'].address, TOTAL_MINTED_EVRS);
            console.log("Created registry -> issuer trust line.");
        }

        const host_lines = await newAccounts['host'].getTrustLines(EvernodeConstants.EVR, newAccounts['issuer'].address);

        if (host_lines.length === 0) {
            await newAccounts['host'].setTrustLine(EvernodeConstants.EVR, newAccounts['issuer'].address, TOTAL_MINTED_EVRS);
            console.log("Created host -> issuer trust line.");
        }

        const tenant_lines = await newAccounts['tenant'].getTrustLines(EvernodeConstants.EVR, newAccounts['issuer'].address);

        if (tenant_lines.length === 0) {
            await newAccounts['tenant'].setTrustLine(EvernodeConstants.EVR, newAccounts['issuer'].address, TOTAL_MINTED_EVRS);
            console.log("Created tenant -> issuer trust line.");
        }
        console.log("Trust Lines initiated");

        // Transfering EVRs 
        await newAccounts['issuer'].makePayment(newAccounts['foundation'].address, TOTAL_MINTED_EVRS, EvernodeConstants.EVR, newAccounts['issuer'].address);

        console.log(`${TOTAL_MINTED_EVRS} EVRs were issued to EVERNODE Foundation`);

        // ISSUER Blackholing
        await newAccounts['issuer'].setRegularKey(BLACKHOLE_ADDRESS);
        await newAccounts['issuer'].setAccountFields({ Flags: { asfDisableMaster: true } });
        console.log("Issuer blackholed");

        // Uploading the hook.
        const api = new xrpljs.Client(server);

        const account = xrpljs.Wallet.fromSeed(genesisSecret)
        await api.connect();
        api.submit(
            {
                Account: account.classicAddress,
                TransactionType: "SetHook",
                Hooks: [
                    {
                        Hook: {
                            CreateCode: fs.readFileSync('hook/accept.wasm').toString('hex').toUpperCase(),
                            HookApiVersion: 0,
                            HookNamespace: "CAFECAFECAFECAFECAFECAFECAFECAFECAFECAFECAFECAFECAFECAFECAFECAFE",
                            HookOn: "0000000000000000",
                            Flags: hsfOVERRIDE
                        }
                    }
                ],
                Fee: "1000000"
            }, { wallet: account }).then(x => {
                console.log(x);
                process.exit(0);

            }).catch(e => console.log(e));

        // Add the error triggering transaction here.---------------------------------------------------
        /**
         * NOTE: If you are performing an EVR transaction via a Host account or a Tenant account,
         * first you have to fund EVRs to that particular account from the Evernode Foundation account.
         */

        // ---------------------------------------------------------------------------------------------

    } catch (error) {
        console.error('Error occured: ', error);

    } finally {
        await xrplApi.disconnect();
        console.log('Disconnected from server');
    }

}
main();