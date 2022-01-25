const { SqliteDatabase, DataTypes } = require('./lib/sqlite-handler');

/**
 * State manager manages the hook states in a SQLITE db.
 */
class StateManager {
    constructor(dbPath) {
        this.db = new SqliteDatabase(dbPath);
    }

    set(key, value) {

    }

    get(key) {

    }
}

module.exports = {
    StateManager
}
