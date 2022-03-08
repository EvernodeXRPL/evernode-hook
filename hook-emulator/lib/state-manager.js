const { SqliteDatabase, DataTypes } = require('./sqlite-handler');
const { StateHelpers } = require('evernode-js-client');

const STATE_TABLE = 'state';
const MAX_KEY_SIZE = 32;
const MAX_VALUE_SIZE = 128;

const STATE_ERROR_CODES = {
    KEY_TOO_BIG: 'KEY_TOO_BIG',
    KEY_TOO_SMALL: 'KEY_TOO_SMALL',
    VALUE_TOO_BIG: 'VALUE_TOO_BIG',
    VALUE_TOO_SMALL: 'VALUE_TOO_SMALL',
    DOES_NOT_EXIST: 'DOES_NOT_EXIST'
}

/**
 * State manager manages the hook states in a SQLITE db.
 */
class StateManager {
    #draftStates = null;
    #db = null;
    #stateTable = null;
    #firestoreManager = null;

    constructor(dbPath, firestoreManager) {
        this.#db = new SqliteDatabase(dbPath);
        this.#firestoreManager = firestoreManager;
        this.#draftStates = {};
    }

    async init() {
        this.#stateTable = STATE_TABLE;

        this.#db.open();
        await this.#db.createTableIfNotExists(this.#stateTable, [
            { name: 'id', type: DataTypes.INTEGER, notNull: true, primary: true },
            { name: 'key', type: DataTypes.BLOB, notNull: true, unique: true },
            { name: 'value', type: DataTypes.BLOB, notNull: true }
        ]);
        this.#db.close();
    }

    set(key, value) {
        if (!key || !key.length || (key.findIndex(e => e != 0) < 0))
            throw { code: STATE_ERROR_CODES.KEY_TOO_SMALL, error: 'State key cannot be empty.' };
        else if (key.length > MAX_KEY_SIZE)
            throw { code: STATE_ERROR_CODES.KEY_TOO_BIG, error: `State key size should be less than ${MAX_KEY_SIZE}.` };
        else if (value.length > MAX_VALUE_SIZE)
            throw { code: STATE_ERROR_CODES.VALUE_TOO_BIG, error: `State value size should be less than ${MAX_VALUE_SIZE}.` };

        let keyBuf = key;
        if (key.length < MAX_KEY_SIZE) {
            keyBuf = Buffer.alloc(MAX_KEY_SIZE, 0);
            key.copy(keyBuf);
        }

        let valueBuf = null;
        if (value && (value.length > 0))
            valueBuf = value;

        this.#draftStates[keyBuf.toString('hex')] = valueBuf;
    }

    async get(key) {
        if (!key || !key.length || (key.findIndex(e => e != 0) < 0))
            throw { code: STATE_ERROR_CODES.KEY_TOO_SMALL, error: 'State key cannot be empty.' };
        else if (key.length > MAX_KEY_SIZE)
            throw { code: STATE_ERROR_CODES.KEY_TOO_BIG, error: `State key size should be less than ${MAX_KEY_SIZE}.` };

        let keyBuf = key;
        if (key.length < MAX_KEY_SIZE) {
            keyBuf = Buffer.alloc(MAX_KEY_SIZE, 0);
            key.copy(keyBuf);
        }

        const hexKey = keyBuf.toString('hex');

        // First check in drafts.
        // Else check in the db.
        if (hexKey in this.#draftStates) {
            if (this.#draftStates[hexKey])
                return this.#draftStates[hexKey];
            else
                throw { code: STATE_ERROR_CODES.DOES_NOT_EXIST, error: `State with this key ${hexKey} does not exist.` };
        }
        else {
            this.#db.open();
            const states = await this.#db.getValues(this.#stateTable, { key: keyBuf });
            this.#db.close();

            if (states && states.length > 0)
                return states[0].value;
            else
                throw { code: STATE_ERROR_CODES.DOES_NOT_EXIST, error: `State with this key ${hexKey} does not exist.` };
        }
    }

    async persist() {
        // Keep updated and deleted indexes in a temprorary lists when updating the state db and then after db is updated persist them to firestore.
        // So firestore update delays won't effect the db update.
        let indexUpdates = {
            set: [],
            delete: []
        }

        this.#db.open();
        for (const [key, value] of Object.entries(this.#draftStates)) {
            const keyBuf = Buffer.from(key, 'hex');
            const states = await this.#db.getValues(this.#stateTable, { key: keyBuf });
            if (value) {
                if (states && states.length > 0) {
                    await this.#db.updateValues(this.#stateTable, {
                        value: value,
                    }, { key: keyBuf });
                }
                else {
                    await this.#db.insertValue(this.#stateTable, {
                        key: keyBuf,
                        value: value
                    });
                }
                indexUpdates.set.push({ keyBuf: keyBuf, value: value });
            }
            else if (states && states.length > 0) {
                await this.#db.deleteValues(this.#stateTable, { key: keyBuf });
                indexUpdates.delete.push({ keyBuf: keyBuf });
            }
        }
        this.#db.close();

        this.#draftStates = {};

        // Persist the data to the firestore index.
        // Since we only have one list object per entry,
        // There's only one insert, update or delete operation per document.
        // So we can promise.all all the inserts,updates and deletes.
        await Promise.all([...indexUpdates.set.map(async obj => {
            await this.setIndex(obj.keyBuf, obj.value).catch(console.error);
        }),
        ...indexUpdates.delete.map(async obj => {
            await this.deleteIndex(obj.keyBuf).catch(console.error);
        })]);
    }

    rollback() {
        this.#draftStates = {};
    }

    async setIndex(stateKey, stateData) {
        const decoded = StateHelpers.decodeStateData(stateKey, stateData);
        if (decoded.type == StateHelpers.StateTypes.HOST_ADDR) {
            delete decoded.type;
            await this.#firestoreManager.setHost(decoded);
        }
        else if (decoded.type == StateHelpers.StateTypes.SIGLETON || decoded.type == StateHelpers.StateTypes.CONFIGURATION)
            await this.#firestoreManager.setConfig(decoded);

    }

    async deleteIndex(stateKey) {
        const decoded = StateHelpers.decodeStateKey(stateKey);
        if (decoded.type == StateHelpers.StateTypes.HOST_ADDR)
            await this.#firestoreManager.deleteHost(decoded.key);
        else if (decoded.type == StateHelpers.StateTypes.SIGLETON || decoded.type == StateHelpers.StateTypes.CONFIGURATION)
            await this.#firestoreManager.deleteConfig(decoded.key);
    }
}

module.exports = {
    StateManager,
    STATE_ERROR_CODES
}
