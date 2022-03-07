const { FirestoreHandler } = require('evernode-js-client');
var fs = require('fs');
const jwt = require('jsonwebtoken');

const SCOPES = [
    "https://www.googleapis.com/auth/datastore"
];

class FirestoreManager extends FirestoreHandler {
    #saKeyPath = null;
    #accessCredentials = null;

    async #authorize() {
        const key = JSON.parse(fs.readFileSync(this.#saKeyPath));

        // Generate the signed JWT.
        const claims = {
            aud: key.token_uri,
            scope: SCOPES.join(' '),
            iss: key.client_email,
            exp: (Date.now() / 1000) + (30 * 60), // Setup exp as 30 minutes.
            iat: (Date.now() / 1000)
        }
        const signedJwt = jwt.sign(JSON.stringify(claims), key.private_key, { algorithm: "RS256" });

        let tokenInfo = await this._sendRequest('POST', key.token_uri, {
            grant_type: 'urn:ietf:params:oauth:grant-type:jwt-bearer',
            assertion: signedJwt
        });

        if (!tokenInfo)
            throw { type: 'Authentication Error', message: 'Unable to generate the access token.' };

        tokenInfo = JSON.parse(tokenInfo);
        this.#accessCredentials = {
            accessToken: tokenInfo.access_token,
            tokenExpiry: Date.now() + (tokenInfo.expires_in * 1000) - (5 * 1000), // Keep 5 second buffer. So access token won't expire while request is processing.
            tokenType: tokenInfo.token_type
        }
    }

    async #sendRequestWithAuth(method, url, params = null, data = null, options = null) {
        return new Promise(async (resolve, reject) => {
            let retryCount = 0;
            const trySend = async () => {
                let reqOptions = options;
                // Set the access headers if access credentials is set.
                if (this.#accessCredentials && this.#accessCredentials.accessToken && this.#accessCredentials.tokenExpiry) {
                    // If token is expired generate a new before setting the header.
                    if (this.#accessCredentials.tokenExpiry < Date.now()) {
                        console.log(`Access token expired, Generating a new...`)
                        await this.#authorize();
                    }
                    const token = `${this.#accessCredentials.tokenType || 'Bearer'} ${this.#accessCredentials.accessToken}`;
                    if (!reqOptions)
                        reqOptions = { headers: { Authorization: token } };
                    else
                        reqOptions.headers = { Authorization: token, ...(reqOptions.headers ? reqOptions.headers : {}) };
                }

                try {
                    const res = await this._sendRequest(method, url, params, data, reqOptions);
                    resolve(res);
                    return;
                }
                catch (e) {
                    // If request is failed with unauthorized status check is token is expired and generate a new token and try sending the request again.
                    // Keep max retries limited to 5.
                    if (res.status === 401) {
                        const resJson = JSON.parse(resData);
                        if (retryCount < 5 &&
                            this.#accessCredentials && this.#accessCredentials.accessToken && this.#accessCredentials.tokenExpiry &&
                            (this.#accessCredentials.tokenExpiry < Date.now()) &&
                            resJson.error.code === 401 && resJson.error.status === 'UNAUTHENTICATED') {
                            retryCount++;
                            console.log(`Access token expired, Generating a new ${retryCount}...`)
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
        // Setup the update fields if an update request.
        // Otherwise set the documentId.
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

        // Prepare the firestore write body with the given data object.
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

        // Prepare the firestore write body with the given data object.
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
        const data = await this.getConfigs({ key: documentId });
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
        const data = await this.getHosts({ key: documentId })
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
