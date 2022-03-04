const { FirestoreHandler, FirestoreOperations, EvernodeConstants } = require('evernode-js-client');

class IndexManager {
    #firestoreHandler = null;

    constructor() {
        this.#firestoreHandler = new FirestoreHandler(EvernodeConstants.INDEX_ID)
    }

    async init(secKeyPath) {
        await this.#firestoreHandler.authorize(secKeyPath);
    }

    async #getConfigs(key = null) {
        return await this.#firestoreHandler.getDocuments(EvernodeConstants.CONFIGS_INDEX,
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
            res = await this.#firestoreHandler.updateDocument(EvernodeConstants.CONFIGS_INDEX, config, documentId);
        else
            res = await this.#firestoreHandler.addDocument(EvernodeConstants.CONFIGS_INDEX, config, documentId);
        return res;
    }

    async deleteConfig(key) {
        if (!key)
            throw { type: 'Validation Error', message: 'Config key is required.' };

        return await this.#firestoreHandler.deleteDocument(EvernodeConstants.CONFIGS_INDEX, key);
    }

    async #getHosts(key = null) {
        return await this.#firestoreHandler.getDocuments(EvernodeConstants.HOSTS_INDEX,
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
            res = await this.#firestoreHandler.updateDocument(EvernodeConstants.HOSTS_INDEX, host, documentId);
        else
            res = await this.#firestoreHandler.addDocument(EvernodeConstants.HOSTS_INDEX, host, documentId);
        return res;
    }

    async deleteHost(key) {
        if (!key)
            throw { type: 'Validation Error', message: 'Host key is required.' };

        return await this.#firestoreHandler.deleteDocument(EvernodeConstants.HOSTS_INDEX, key);
    }
}

module.exports = {
    IndexManager
}