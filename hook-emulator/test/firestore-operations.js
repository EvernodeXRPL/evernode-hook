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
    for (let host of hosts) {
        if (host.noOfActiveInstances && host.noOfTotalInstances) {
            console.log(`Updating host ${host.address}...`);
            host.activeInstances = host.noOfActiveInstances;
            delete host.noOfActiveInstances;
            host.maxInstances = host.noOfTotalInstances;
            delete host.noOfTotalInstances;
            await firestoreManager.deleteHost(host.key);
            await firestoreManager.setHost(host);
            console.log(`Updated host ${host.address}.`);
        }
    }
}

const main = async () => {
    await executeFunc(DEV_REGISTRY_ADDRESS, updateInsCountKeys);
}

main().catch(console.error);
