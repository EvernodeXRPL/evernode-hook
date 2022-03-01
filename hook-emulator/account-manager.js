const fs = require('fs');

const FILE_NAME = 'hook-root.json';

/**
 * Root data manager manages the hook root values in a json file.
 * This can be removed once fetching sfMintedTokens from the xrpl lib is implemented.
 */
class AccountManager {
    #jsonFile = null;
    #rootData = null;
    #initialized = false;

    constructor() {
        this.#rootData = {
            sfMintedTokens: 0
        };
        this.#jsonFile = FILE_NAME;
    }

    init() {
        // Check if data file exists, write otherwise.
        if (!fs.existsSync(this.#jsonFile))
            fs.writeFileSync(this.#jsonFile, JSON.stringify(this.#rootData, null, 4));
        else
            this.#rootData = JSON.parse(fs.readFileSync(this.#jsonFile).toString());
        this.#initialized = true;
    }

    increaseMintedTokensSeq() {
        if (!this.#initialized)
            throw 'Account manager is not initialized.'

        this.#rootData.sfMintedTokens++;
    }

    decreaseMintedTokensSeq() {
        if (!this.#initialized)
            throw 'Account manager is not initialized.'

        this.#rootData.sfMintedTokens--;
    }

    getMintedTokensSeq() {
        if (!this.#initialized)
            throw 'Account manager is not initialized.'

        return this.#rootData.sfMintedTokens;
    }

    persist() {
        if (!this.#initialized)
            throw 'Account manager is not initialized.'

        fs.writeFileSync(this.#jsonFile, JSON.stringify(this.#rootData, null, 4));
    }

    rollback() {
        if (!this.#initialized)
            throw 'Account manager is not initialized.'

        this.#rootData = JSON.parse(fs.readFileSync(this.#jsonFile).toString());
    }
}

module.exports = {
    AccountManager
}
