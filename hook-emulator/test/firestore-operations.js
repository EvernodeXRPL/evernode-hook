const evernode = require('evernode-js-client');
const { FirestoreManager } = require('../lib/firestore-manager');

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

const main = async () => {
    await executeFunc(DEV_REGISTRY_ADDRESS, updateInsCountKeys);
}

main().catch(console.error);
