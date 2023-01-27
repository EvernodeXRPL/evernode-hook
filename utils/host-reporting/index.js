const fs = require('fs');
const path = require('path');
const evernode = require("evernode-js-client");

const CONFIG_PATH = 'reporting.cfg';
const DATA_DIR = 'data';

if (!fs.existsSync(CONFIG_PATH)) {
    fs.writeFileSync(CONFIG_PATH, JSON.stringify({
        "hookAddress": ""
    }));
    console.log('Populate the configs and run again!');
    process.exit(0);
}

const cfg = JSON.parse(fs.readFileSync(CONFIG_PATH));

const hookAddress = cfg.hookAddress;

const app = async () => {
    const xrplApi = new evernode.XrplApi('wss://hooks-testnet-v2.xrpl-labs.com');
    evernode.Defaults.set({
        registryAddress: hookAddress,
        xrplApi: xrplApi
    })

    try {
        await xrplApi.connect();

        const hosts = await getHosts();
        const path = generateReport(hosts);

        console.log(`Generated the report and saved in location: \n${path}`);
    }
    catch (e) {
        console.error("Error occured:", e);
    }
    finally {
        await xrplApi.disconnect();
    }
}

const getHosts = async () => {
    const registryClient = new evernode.RegistryClient();
    await registryClient.connect();
    const hosts = await registryClient.getAllHosts();
    await registryClient.disconnect();

    const infos = await Promise.all(hosts.map(async (host) => {
        const hostClient = new evernode.HostClient(host.address);
        await hostClient.connect();
        const info = {
            ...(await hostClient.getHostInfo()),
            evrBalance: await hostClient.getEVRBalance(),
            xrpBalance: (await hostClient.xrplAcc.getInfo()).Balance / 1000000,
            vacantLeaseCount: (await hostClient.getLeaseOffers()).length,
            domain: await hostClient.xrplAcc.getDomain(),
        }
        await hostClient.disconnect();
        return info;
    }));
    return infos;
}

const generateReport = (hosts) => {
    const keys = Object.keys(hosts.sort((a, b) => Object.keys(b).length - Object.keys(a).length)[0]);
    const content = [
        keys.map(k => `"${k ? (k.charAt(0).toUpperCase() + k.slice(1)) : ''}"`).join(','),
        ...hosts.map(h => keys.map(k => `"${isNaN(h[k]) ? (h[k] || '') : h[k]}"`).join(','))
    ].join('\n');
    if (!fs.existsSync(DATA_DIR))
        fs.mkdirSync(DATA_DIR);

    const filePath = path.resolve(DATA_DIR, `${hookAddress}_hosts_${Date.now()}.csv`);
    fs.writeFileSync(filePath, content);

    return filePath;
}

app().catch(console.error);