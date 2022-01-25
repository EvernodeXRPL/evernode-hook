const { SqliteDatabase, DataTypes } = require('./lib/sqlite-handler');

const STATE_TABLE = 'state';
const HEX_REGEXP = new RegExp(/^[0-9A-Fa-f]+$/);

class StateManager {
    #draftStates = null;
    #db = null;
    #stateTable = null;

    constructor(dbPath) {
        this.#db = new SqliteDatabase(dbPath);
        this.#draftStates = {};
    }

    async init() {
        this.#stateTable = STATE_TABLE;

        this.#db.open();
        await this.#db.createTableIfNotExists(this.#stateTable, [
            { name: 'id', type: DataTypes.INTEGER, notNull: true, primary: true },
            { name: 'key', type: DataTypes.TEXT, notNull: true, unique: true },
            { name: 'value', type: DataTypes.TEXT, notNull: true }
        ]);
        this.#db.close();
    }

    set(key, value) {
        if (!key)
            throw 'State key cannot be empty.';

        if (!HEX_REGEXP.test(key) || (value && !HEX_REGEXP.test(value)))
            throw 'State key and value should be hex.';

        key = key.toUpperCase();
        value = value ? value.toUpperCase() : null;

        this.#draftStates[key] = value;
    }

    async get(key) {
        if (!key)
            throw 'State key cannot be empty.';

        if (!HEX_REGEXP.test(key))
            throw 'State key should be hex.';

        key = key.toUpperCase();

        // First check in drafts.
        // Else check in the db.
        if (key in this.#draftStates)
            return this.#draftStates[key] ? this.#draftStates[key] : null;
        else {
            this.#db.open();
            const states = await this.#db.getValues(this.#stateTable, { key: key });
            this.#db.close();

            return (states && states.length > 0) ? states[0].value : null;
        }
    }

    async persist() {
        this.#db.open();
        for (const key of Object.keys(this.#draftStates)) {
            const states = await this.#db.getValues(this.#stateTable, { key: key });
            if (this.#draftStates[key]) {
                if (states && states.length > 0) {
                    await this.#db.updateValues(this.#stateTable, {
                        value: this.#draftStates[key],
                    }, { key: key });
                }
                else {
                    await this.#db.insertValue(this.#stateTable, {
                        key: key,
                        value: this.#draftStates[key]
                    });
                }
            }
            else if (states && states.length > 0) {
                await this.#db.deleteValues(this.#stateTable, { key: key });
            }
        }
        this.#db.close();

        this.#draftStates = {};
    }

    rollback() {
        this.#draftStates = {};
    }
}

module.exports = {
    StateManager
}
