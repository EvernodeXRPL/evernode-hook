const sqlite3 = require('sqlite3').verbose();
const codec = require('ripple-address-codec');

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

const deleteStateValue = async (db, hexKey) => {
    const key = Buffer.from(hexKey, 'hex');
    const query = `DELETE FROM state WHERE key = ?;`;
    return new Promise((resolve, reject) => {
        db.run(query, [key], function (err) {
            if (err) {
                reject(err);
                return;
            }
            resolve({ changes: this.changes });
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

const deletHosts = async (db) => {
    const addresses = [
        'rpCqZSTN9YkwV5knsbujGRE1Q37mRTwH7T',
        'rs9YfGnrgkqLXsxh1MUPfkvSiwkYEY6yhw',
        'rs65gGVF1cJCRrCphpYBjTRWgHab91AHkw',
        'rnynbxsbC1pBcYTUumQq6zyyejVga7KuHD',
        'rnynbxsbC1pBcYTUumQq6zyyejVga7KuHD',
        'rHresLrv5cn2HtbJkfvPQa1HDrYZxBFXuw',
        'rJcPq2LrTxMgbtXta9zgaHbjt2zMyGT56x',
        'rKKGyagwAW2UFoZ5voeQztoqWmm6zJorbY',
        'rMD7bxfBMbmaCaVmA5Qf5YnpUmCnCjmue5',
        'rPSDgdEAsL3LRmxthqj3TECuR4rpSToogz'
    ];
    const countKey = '4556523200000000000000000000000000000000000000000000000000000000';
    let delCount = 0;
    for (const address of addresses) {
        const accKey = Buffer.from('4556520300000000000000000000000000000000000000000000000000000000', 'hex');
        codec.decodeAccountID(address).copy(accKey, accKey.length - 20);
        const addrVal = await getStateValue(db, accKey.toString('hex'));
        if (addrVal.length) {
            const idKey = Buffer.from('4556520200000000000000000000000000000000000000000000000000000000', 'hex');
            addrVal[0].value.copy(idKey, 4, 4);
            await deleteStateValue(db, idKey.toString('hex'));
            delCount++;
        }
        await deleteStateValue(db, accKey.toString('hex'));
    }
    const countVal = await getStateValue(db, countKey);
    const count = countVal[0].value.readUInt32BE();
    const buf = Buffer.allocUnsafe(4);
    buf.writeUInt32BE(count - delCount);
    await setStateValue(db, countKey, buf.toString('hex'));
}

const main = async () => {
    // await executeFunc(DB_FILE, updateHostCount);
    // await executeFunc(DB_FILE, deletHosts);    
}

main().catch(console.error);
