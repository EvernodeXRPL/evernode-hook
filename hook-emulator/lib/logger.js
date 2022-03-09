const fs = require('fs');
const path = require('path');
const util = require('util');

const formatText = (text, logType = 'dbg') => {
    const date = new Date().toISOString().
        replace(/T/, ' ').       // Replace T with a space.
        replace(/\..+/, '').     // Delete the dot and everything after.
        replace(/-/g, '');     // Delete the dashes.

    return `${date} [${logType}] ${text}\n`;
}

exports.init = (logPath, fileLogEnabled) => {

    let flog = null;

    if (fileLogEnabled) {
        const dirname = path.dirname(logPath);
        if (!fs.existsSync(dirname))
            fs.mkdirSync(dirname, { recursive: true });

        flog = fs.createWriteStream(logPath, { flags: 'a' });
    }

    console.log = function () {
        const text = formatText(util.format.apply(this, arguments));
        process.stdout.write(text);

        if (flog)
            flog.write(text);
    };
    console.error = function () {
        const text = formatText(util.format.apply(this, arguments), 'err');
        process.stderr.write(text);

        if (flog)
            flog.write(text);
    };
}