const { SqliteDatabase, DataTypes } = require('./lib/sqlite-handler');

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
