const evernode = require('evernode-js-client');
const { FirestoreManager } = require('../lib/firestore-manager');
const codec = require('ripple-address-codec');

const FIREBASE_SEC_KEY_PATH = '../sec/firebase-sa-key.json';
const DEV_REGISTRY_ADDRESS = 'rDsg8R6MYfEB7Da861ThTRzVUWBa3xJgWL';
const BETA_REGISTRY_ADDRESS = 'rHQQq5aJ5kxFyNJXE36rAmuhxpDvpLHcWq';

const executeFunc = async (registryAddress, callback) => {
    evernode.Defaults.set({
        registryAddress: registryAddress
    })
    const firestoreManager = new FirestoreManager();
    await firestoreManager.authorize(FIREBASE_SEC_KEY_PATH);
    await callback(firestoreManager);
}

// Registry operations here.
const updateInsCountKeys = async (firestoreManager) => {
    const hosts = await firestoreManager.getHosts();
    for (const host of hosts) {
        if (host.hasOwnProperty('noOfActiveInstances') && host.hasOwnProperty('noOfTotalInstances')) {
            console.log(`Updating host ${host.address}...`);
            let temp = JSON.parse(JSON.stringify(host));
            temp.activeInstances = temp.noOfActiveInstances;
            delete temp.noOfActiveInstances;
            temp.maxInstances = temp.noOfTotalInstances;
            delete temp.noOfTotalInstances;
            await firestoreManager.deleteHost(temp.key);
            try {
                await firestoreManager.setHost(temp);
                console.log(`Updated host ${temp.address}.`);
            }
            catch (e) {
                console.error(e);
                await firestoreManager.setHost(host);
                console.log(`Rolled back host ${host.address}...`);
            }
        }
    }
}

const deleteHosts = async (firestoreManager) => {
    const addresses = [
        'rpCqZSTN9YkwV5knsbujGRE1Q37mRTwH7T',
        'rs9YfGnrgkqLXsxh1MUPfkvSiwkYEY6yhw',
        'rs65gGVF1cJCRrCphpYBjTRWgHab91AHkw',
        'rnynbxsbC1pBcYTUumQq6zyyejVga7KuHD',
        'rHresLrv5cn2HtbJkfvPQa1HDrYZxBFXuw',
        'rJcPq2LrTxMgbtXta9zgaHbjt2zMyGT56x',
        'rKKGyagwAW2UFoZ5voeQztoqWmm6zJorbY',
        'rMD7bxfBMbmaCaVmA5Qf5YnpUmCnCjmue5',
        'rPSDgdEAsL3LRmxthqj3TECuR4rpSToogz'
    ];
    const countKey = '4556523200000000000000000000000000000000000000000000000000000000';
    for (const address of addresses) {
        const accKey = Buffer.from('4556520300000000000000000000000000000000000000000000000000000000', 'hex');
        codec.decodeAccountID(address).copy(accKey, accKey.length - 20);
        const host = await firestoreManager.getHosts({ key: accKey.toString('hex').toUpperCase() });
        await firestoreManager.deleteHost(host[0].key);
    }
}

const main = async () => {
    // await executeFunc(DEV_REGISTRY_ADDRESS, updateInsCountKeys);
    // await executeFunc(DEV_REGISTRY_ADDRESS, deleteHosts);
}

main().catch(console.error);
