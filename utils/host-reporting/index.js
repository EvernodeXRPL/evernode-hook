const fs = require('fs');
const path = require('path');
const evernode = require("evernode-js-client");
const { appenv } = require('../../common');

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
    const xrplApi = new evernode.XrplApi('wss://hooks-testnet-v3.xrpl-labs.com');
    evernode.Defaults.set({
        registryAddress: hookAddress,
        xrplApi: xrplApi,
        NetworkID: appenv.NETWORK_ID
    })

    try {
        await xrplApi.connect();

        const client = new evernode.RegistryClient();

        await client.connect();
        const hosts = await getHosts(client);
        await client.disconnect();

        const path = generateReport(hosts, client.config);

        console.log(`Generated the report and saved in location: \n${path}`);
    }
    catch (e) {
        console.error("Error occured:", e);
    }
    finally {
        await xrplApi.disconnect();
    }
}

const getHosts = async (client) => {
    const hosts = await client.getAllHosts();

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
    return infos.sort((a, b) => b.active - a.active);
}

const generateReport = (hosts, config) => {
    function formatText(value, upperFirst = false) {
        let final;
        if (typeof value == 'string') {
            final = value || '';
            if (final && upperFirst)
                final = final.charAt(0).toUpperCase() + final.slice(1)
        }
        else if (typeof value == 'object')
            final = JSON.stringify(value, null, 2);
        else
            final = (value || value === 0) ? value.toString() : (value === false) ? 'false' : '';
        return `"${final ? final.replace(/"/g, "'") : ''}"`;
    }

    const hostKeys = Object.keys(hosts.sort((a, b) => Object.keys(b).length - Object.keys(a).length)[0]);
    const hostContent = [
        hostKeys.map(k => formatText(k, true)).join(','),
        ...hosts.map(h => hostKeys.map(k => formatText(h[k])).join(','))
    ].join('\n');

    const configContent = Object.entries(config).map(([key, value]) =>
        `${formatText(key)},${formatText(value)}`
    ).join('\n');

    const content = `${hostContent}\n\n\n${configContent}`;

    if (!fs.existsSync(DATA_DIR))
        fs.mkdirSync(DATA_DIR);

    const filePath = path.resolve(DATA_DIR, `${hookAddress}_hosts_${Date.now()}.csv`);
    fs.writeFileSync(filePath, content);

    return filePath;
}

app().catch(console.error);