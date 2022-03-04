const { FirestoreHandler, FirestoreOperations } = require('evernode-js-client');

const FirestoreCollections = {
    CONFIGS: 'configs',
    HOSTS: 'hosts'
}

class IndexManager {
    #firestoreHandler = null;

    constructor(projectId) {
        this.#firestoreHandler = new FirestoreHandler(projectId)
    }

    async init(secKeyPath) {
        await this.#firestoreHandler.authorize(secKeyPath);
    }

    async #getConfigs(key = null) {
        return await this.#firestoreHandler.getDocuments(FirestoreCollections.CONFIGS,
            key ? { list: [{ key: key, operator: FirestoreOperations.EQUAL }] } : null);
    }

    async setConfig(config) {
        if (!config.key || !config.hasOwnProperty('value') || !config.type)
            throw { type: 'Validation Error', message: 'Config key, value and type are required.' };

        // If document already exist, update that.
        const documentId = config.key;
        const data = await this.#getConfigs(documentId);
        let res;
        if (data && data.length)
            res = await this.#firestoreHandler.updateDocument(FirestoreCollections.CONFIGS, config, documentId);
        else
            res = await this.#firestoreHandler.addDocument(FirestoreCollections.CONFIGS, config, documentId);
        return res;
    }

    async deleteConfig(key) {
        if (!key)
            throw { type: 'Validation Error', message: 'Config key is required.' };

        return await this.#firestoreHandler.deleteDocument(FirestoreCollections.CONFIGS, key);
    }

    async #getHosts(key = null) {
        return await this.#firestoreHandler.getDocuments(FirestoreCollections.HOSTS,
            key ? { list: [{ key: key, operator: FirestoreOperations.EQUAL }] } : null);
    }

    async setHost(host) {
        if (!host.key)
            throw { type: 'Validation Error', message: 'Host key is required.' };

        // If document already exist, update that.
        const documentId = host.key;
        const data = await this.#getHosts(documentId);
        let res;
        if (data && data.length)
            res = await this.#firestoreHandler.updateDocument(FirestoreCollections.HOSTS, host, documentId);
        else
            res = await this.#firestoreHandler.addDocument(FirestoreCollections.HOSTS, host, documentId);
        return res;
    }

    async deleteHost(key) {
        if (!key)
            throw { type: 'Validation Error', message: 'Host key is required.' };

        return await this.#firestoreHandler.deleteDocument(FirestoreCollections.HOSTS, key);
    }
}

module.exports = {
    IndexManager
}