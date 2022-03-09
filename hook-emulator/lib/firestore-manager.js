const { FirestoreHandler } = require('evernode-js-client');
var fs = require('fs');
const jwt = require('jsonwebtoken');

const SCOPES = [
    "https://www.googleapis.com/auth/datastore"
];

const TOKEN_EXPIRY_BUFFER_SECONDS = 5;
const JWT_EXPIRY_MINUTES = 5;

class FirestoreManager extends FirestoreHandler {
    #saKeyPath = null;
    #accessCredentials = null;
    #authPromise = null;

    async #authorize() {
        // Prevent calling autherization multiple times on parallel request sendings.
        // If authPromise is set which means there's ongoing authorization,
        // return it directly, Otherwise create an authPromise.
        if (this.#authPromise)
            return this.#authPromise;
        else {
            this.#authPromise = new Promise(async (resolve, reject) => {
                console.log(`Generating a new firestore access token...`)
                const key = JSON.parse(fs.readFileSync(this.#saKeyPath));

                // Generate the signed JWT.
                const claims = {
                    aud: key.token_uri,
                    scope: SCOPES.join(' '),
                    iss: key.client_email,
                    exp: (Date.now() / 1000) + (JWT_EXPIRY_MINUTES * 60),
                    iat: (Date.now() / 1000)
                }
                const signedJwt = jwt.sign(JSON.stringify(claims), key.private_key, { algorithm: "RS256" });

                let tokenInfo = await this.sendRequest('POST', key.token_uri, {
                    grant_type: 'urn:ietf:params:oauth:grant-type:jwt-bearer',
                    assertion: signedJwt
                });

                if (!tokenInfo) {
                    reject({ type: 'Authentication Error', message: 'Unable to generate the access token.' });
                    return;
                }

                tokenInfo = JSON.parse(tokenInfo);
                this.#accessCredentials = {
                    accessToken: tokenInfo.access_token,
                    tokenExpiry: Date.now() + (tokenInfo.expires_in * 1000),
                    tokenType: tokenInfo.token_type
                }
                resolve();
                // Make the authPromise null when authorization completed.
                this.#authPromise = null;
            });
            return this.#authPromise;
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
                    // Keep some buffer time without waiting until the last moment to renew the token.
                    // So access token won't expire while request is processing.
                    if ((this.#accessCredentials.tokenExpiry - (TOKEN_EXPIRY_BUFFER_SECONDS * 1000)) < Date.now())
                        await this.#authorize();

                    const token = `${this.#accessCredentials.tokenType || 'Bearer'} ${this.#accessCredentials.accessToken}`;
                    if (!reqOptions)
                        reqOptions = { headers: { Authorization: token } };
                    else
                        reqOptions.headers = { Authorization: token, ...(reqOptions.headers ? reqOptions.headers : {}) };
                }

                try {
                    const res = await this.sendRequest(method, url, params, data, reqOptions);
                    resolve(res);
                    return;
                }
                catch (e) {
                    // If request is failed with unauthorized status, check if token is expired, if so try sending the request again,
                    // So next try will generate a new token.
                    // Keep max retries limited to 5.
                    if (e && e.status === 401) {
                        const resJson = JSON.parse(e.data);
                        if (retryCount < 5 &&
                            this.#accessCredentials && this.#accessCredentials.accessToken && this.#accessCredentials.tokenExpiry &&
                            (this.#accessCredentials.tokenExpiry < Date.now()) &&
                            resJson.error.code === 401 && resJson.error.status === 'UNAUTHENTICATED') {
                            retryCount++;
                            console.log(`Access token expired, Retrying ${retryCount}...`)
                            await trySend();
                            return;
                        }
                    }
                    reject(e);
                    return;
                }
            }

            await trySend();
        });
    }

    async #write(collectionId, document, documentId, update = false) {
        if (!collectionId || !document || !documentId)
            throw { type: 'Validation Error', message: 'collectionId, document and documentId are required' };

        const url = this.buildApiPath(collectionId, update && documentId);
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

        const url = this.buildApiPath(collectionId, documentId);
        return await this.#sendRequestWithAuth('DELETE', url);
    }

    async #addDocument(collectionId, data, documentId) {
        let document = {
            fields: {}
        };

        // Prepare the firestore write body with the given data object.
        for (const [key, value] of Object.entries(data)) {
            const field = this.convertValue(key, value)
            document.fields[field.key] = field.value;
        }

        let res = await this.#write(collectionId, document, documentId);
        res = res ? JSON.parse(res) : {};
        return this.parseDocument(res);
    }

    async #updateDocument(collectionId, data, documentId) {
        let document = {
            fields: {}
        };

        // Prepare the firestore write body with the given data object.
        for (const [key, value] of Object.entries(data)) {
            const field = this.convertValue(key, value)
            document.fields[field.key] = field.value;
        }

        let res = await this.#write(collectionId, document, documentId, true);
        res = res ? JSON.parse(res) : {};
        return this.parseDocument(res);
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
            res = await this.#updateDocument(this.getCollectionId('configs'), config, documentId);
        else
            res = await this.#addDocument(this.getCollectionId('configs'), config, documentId);
        return res;
    }

    async deleteConfig(key) {
        if (!key)
            throw { type: 'Validation Error', message: 'Config key is required.' };

        return await this.#deleteDocument(this.getCollectionId('configs'), key);
    }

    async setHost(host) {
        if (!host.key)
            throw { type: 'Validation Error', message: 'Host key is required.' };

        // If document already exist, update that.
        const documentId = host.key;
        const data = await this.getHosts({ key: documentId })
        let res;
        if (data && data.length)
            res = await this.#updateDocument(this.getCollectionId('hosts'), host, documentId);
        else
            res = await this.#addDocument(this.getCollectionId('hosts'), host, documentId);
        return res;
    }

    async deleteHost(key) {
        if (!key)
            throw { type: 'Validation Error', message: 'Host key is required.' };

        return await this.#deleteDocument(this.getCollectionId('hosts'), key);
    }
}

module.exports = {
    FirestoreManager
}
