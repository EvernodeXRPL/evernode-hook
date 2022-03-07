const { FirestoreHandler, FirestoreOperations } = require('evernode-js-client');
const { google } = require("googleapis");
var fs = require('fs');

const SCOPES = [
    "https://www.googleapis.com/auth/datastore"
];

class FirestoreManager extends FirestoreHandler {
    #saKeyPath = null;
    #accessToken = null;
    #tokenExpiry = null;

    async #authorize() {
        const key = JSON.parse(fs.readFileSync(this.#saKeyPath));
        const jwtClient = new google.auth.JWT(
            key.client_email,
            null,
            key.private_key,
            SCOPES,
            null
        );
        return new Promise((resolve, reject) => {
            jwtClient.authorize((err, tokens) => {
                if (err) {
                    reject(err);
                    return;
                }
                this.#accessToken = tokens.access_token;
                this.#tokenExpiry = tokens.expiry_date;
                resolve();
                return;
            });
        });
    }

    async #sendRequestWithAuth(method, url, params = null, data = null, options = null) {
        return new Promise(async (resolve, reject) => {
            let retryCount = 0;
            const trySend = async () => {
                let reqOptions = options;
                if (this.#accessToken && this.#tokenExpiry) {
                    if (this.#tokenExpiry < (new Date().getTime())) {
                        console.log(`Invalid access token, Generating a new...`)
                        await this.#authorize();
                    }
                    const bearer = `Bearer ${this.#accessToken}`;
                    if (!reqOptions)
                        reqOptions = { headers: { Authorization: bearer } };
                    else
                        reqOptions.headers = { Authorization: bearer, ...(reqOptions.headers ? reqOptions.headers : {}) };
                }

                try {
                    const res = await this._sendRequest(method, url, params, data, reqOptions);
                    resolve(res);
                    return;
                }
                catch (e) {
                    if (res.status === 401) {
                        const resJson = JSON.parse(resData);
                        if (retryCount < 5 && this.#accessToken && this.#tokenExpiry && (this.#tokenExpiry < (new Date().getTime())) &&
                            resJson.error.code === 401 && resJson.error.status === 'UNAUTHENTICATED') {
                            retryCount++;
                            console.log(`Invalid access token, Retrying ${retryCount}...`)
                            await this.#authorize();
                            await trySend();
                            return;
                        }
                    }
                    reject(e);
                    return;
                }
            }

            trySend();
        });
    }

    async #write(collectionId, document, documentId, update = false) {
        if (!collectionId || !document || !documentId)
            throw { type: 'Validation Error', message: 'collectionId, document and documentId are required' };

        const url = this._buildApiPath(collectionId, update && documentId);
        let params = null;
        if (update)
            params = { "updateMask.fieldPaths": Object.keys(document.fields) };
        else
            params = { documentId: documentId }
        return await this.#sendRequestWithAuth(update ? 'PATCH' : 'POST', url, params, document);
    }

    async #delete(collectionId, documentId) {
        if (!collectionId || !documentId)
            throw { type: 'Validation Error', message: 'collectionId and documentId is required' };

        const url = this._buildApiPath(collectionId, documentId);
        return await this.#sendRequestWithAuth('DELETE', url);
    }

    async #addDocument(collectionId, data, documentId) {
        let document = {
            fields: {}
        };

        for (const [key, value] of Object.entries(data)) {
            const field = this._convertValue(key, value)
            document.fields[field.key] = field.value;
        }

        let res = await this.#write(collectionId, document, documentId);
        res = res ? JSON.parse(res) : {};
        return this._parseDocument(res);
    }

    async #updateDocument(collectionId, data, documentId) {
        let document = {
            fields: {}
        };

        for (const [key, value] of Object.entries(data)) {
            const field = this._convertValue(key, value)
            document.fields[field.key] = field.value;
        }

        let res = await this.#write(collectionId, document, documentId, true);
        res = res ? JSON.parse(res) : {};
        return this._parseDocument(res);
    }

    async #deleteDocument(collectionId, documentId) {
        let res = await this.#delete(collectionId, documentId);
        if (res)
            return `Successfully deleted document ${collectionId}/${documentId}`;
        return false;
    }

    async authorize(saKeyPath) {
        this.#saKeyPath = saKeyPath;
        await this.#authorize();
    }

    async setConfig(config) {
        if (!config.key || !config.hasOwnProperty('value') || !config.type)
            throw { type: 'Validation Error', message: 'Config key, value and type are required.' };

        // If document already exist, update that.
        const documentId = config.key;
        const data = await this.getConfigs({ list: [{ key: documentId, operator: FirestoreOperations.EQUAL }] });
        let res;
        if (data && data.length)
            res = await this.#updateDocument(this._getCollectionId('configs'), config, documentId);
        else
            res = await this.#addDocument(this._getCollectionId('configs'), config, documentId);
        return res;
    }

    async deleteConfig(key) {
        if (!key)
            throw { type: 'Validation Error', message: 'Config key is required.' };

        return await this.#deleteDocument(this._getCollectionId('configs'), key);
    }

    async setHost(host) {
        if (!host.key)
            throw { type: 'Validation Error', message: 'Host key is required.' };

        // If document already exist, update that.
        const documentId = host.key;
        const data = await this.getHosts({ list: [{ key: documentId, operator: FirestoreOperations.EQUAL }] })
        let res;
        if (data && data.length)
            res = await this.#updateDocument(this._getCollectionId('hosts'), host, documentId);
        else
            res = await this.#addDocument(this._getCollectionId('hosts'), host, documentId);
        return res;
    }

    async deleteHost(key) {
        if (!key)
            throw { type: 'Validation Error', message: 'Host key is required.' };

        return await this.#deleteDocument(this._getCollectionId('hosts'), key);
    }
}

module.exports = {
    FirestoreManager
}
