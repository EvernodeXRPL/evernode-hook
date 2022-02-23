const { DataTypes } = require('./lib/sqlite-handler');

const ACCOUNT_TABLE = 'account';

const SFCODES = {
    sfMintedTokens: 'sfMintedTokens'
}

/**
 * Account manager manages the hook root values in a SQLITE db.
 * This can be removed once fetching sfMintedTokens from the xrpl lib is implemented.
 */
class AccountManager {
    #mintedTokensSeq = null;
    #db = null;
    #accountTable = null;

    constructor(db) {
        this.#db = db;
    }

    async init() {
        this.#accountTable = ACCOUNT_TABLE;

        this.#db.open();
        await this.#db.createTableIfNotExists(this.#accountTable, [
            { name: 'id', type: DataTypes.INTEGER, notNull: true, primary: true },
            { name: 'sfcode', type: DataTypes.TEXT, notNull: true, unique: true },
            { name: 'value', type: DataTypes.BLOB, notNull: true }
        ]);
        const values = await this.#db.getValues(this.#accountTable, { sfcode: SFCODES.sfMintedTokens });
        if (values && values.length > 0)
            this.#mintedTokensSeq = values[0].value.readUInt32BE();
        else {
            this.#mintedTokensSeq = 0;
            let buf = Buffer.allocUnsafe(4);
            buf.writeUInt32BE(this.#mintedTokensSeq);
            await this.#db.insertValue(this.#accountTable, { sfcode: SFCODES.sfMintedTokens, value: buf });
        }
        this.#db.close();
    }

    incrementMintedTokensSeq() {
        if (this.#mintedTokensSeq != 0 && !this.#mintedTokensSeq)
            throw 'Account manager is not initialized.'

        this.#mintedTokensSeq++;
    }

    getMintedTokensSeq() {
        if (this.#mintedTokensSeq != 0 && !this.#mintedTokensSeq)
            throw 'Account manager is not initialized.'

        return this.#mintedTokensSeq;
    }

    async persist() {
        if (this.#mintedTokensSeq != 0 && !this.#mintedTokensSeq)
            throw 'Account manager is not initialized.'

        let buf = Buffer.allocUnsafe(4);
        buf.writeUInt32BE(this.#mintedTokensSeq);
        this.#db.open();
        await this.#db.updateValues(this.#accountTable, { value: buf }, { sfcode: SFCODES.sfMintedTokens });
        this.#db.close();
    }

    async rollback() {
        if (this.#mintedTokensSeq != 0 && !this.#mintedTokensSeq)
            throw 'Account manager is not initialized.'

        this.#db.open();
        const values = await this.#db.getValues(this.#accountTable, { sfcode: SFCODES.sfMintedTokens });
        this.#db.close();
        
        if (values && values.length > 0)
            this.#mintedTokensSeq = values[0].value.readUInt32BE();
        else
            this.#mintedTokensSeq = 0;
    }
}

module.exports = {
    AccountManager
}
