const sqlite3 = require('sqlite3').verbose();

const DB_FILE = 'hook-emulator.sqlite';

const getStateValue = async (db, hexKey) => {
    const key = Buffer.from(hexKey, 'hex');
    const query = `SELECT * FROM state WHERE key = ?;`;
    return new Promise((resolve, reject) => {
        let rows = [];
        db.each(query, [key], function (err, row) {
            if (err) {
                reject(err);
                return;
            }
            rows.push(row);
        }, function (err) {
            if (err) {
                reject(err);
                return;
            }
            resolve(rows);
        });
    });
}

const setStateValue = async (db, hexKey, hexValue) => {
    const key = Buffer.from(hexKey, 'hex');
    const value = Buffer.from(hexValue, 'hex');
    const query = `UPDATE state SET value = ? WHERE key = ?;`;
    return new Promise((resolve, reject) => {
        db.run(query, [value, key], function (err) {
            if (err) {
                reject(err);
                return;
            }
            resolve({ lastId: this.lastID, changes: this.changes });
        });
    });
}

const executeFunc = async (dbFile, callback) => {
    const db = new sqlite3.Database(dbFile);
    await callback(db);
    db.close();
}

// DB operations here.
const updateHostCount = async (db) => {
    const hexKey = '4556523200000000000000000000000000000000000000000000000000000000'
    const value = await getStateValue(db, hexKey);
    console.log(value);
    const buf = Buffer.allocUnsafe(4);
    buf.writeUInt32BE(7);
    await setStateValue(db, hexKey, buf.toString('hex'));
}

const main = async () => {
    await executeFunc(DB_FILE, updateHostCount);
}

main().catch(console.error);
