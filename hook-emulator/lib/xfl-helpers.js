const minMantissa = 1000000000000000n
const maxMantissa = 9999999999999999n
const minExponent = -96
const maxExponent = 80

// Helper class to handle XFL float numbers.
class XflHelpers {

    static getExponent(xfl) {
        if (xfl < 0n)
            throw "Invalid XFL";
        if (xfl == 0n)
            return 0n;
        return ((xfl >> 54n) & 0xFFn) - 97n;
    }

    static getMantissa(xfl) {
        if (xfl < 0n)
            throw "Invalid XFL";
        if (xfl == 0n)
            return 0n;
        return xfl - ((xfl >> 54n) << 54n);
    }

    static isNegative(xfl) {
        if (xfl < 0n)
            throw "Invalid XFL";
        if (xfl == 0n)
            return false;
        return ((xfl >> 62n) & 1n) == 0n;
    }

    static toString(xfl) {
        if (xfl < 0n)
            throw "Invalid XFL";
        if (xfl == 0n)
            return '0';

        const mantissa = this.getMantissa(xfl);
        const exponent = this.getExponent(xfl);
        const mantissaStr = mantissa.toString();
        let finalResult = '';
        if (exponent > 0n) {
            finalResult = mantissaStr.padEnd(mantissaStr.length + Number(exponent), '0');
        } else {
            const newExponent = Number(exponent) + mantissaStr.length;
            const cleanedMantissa = mantissaStr.replace(/0+$/, '');
            if (newExponent == 0) {
                finalResult = '0.' + cleanedMantissa;
            } else if (newExponent < 0) {
                finalResult = '0.' + cleanedMantissa.padStart(newExponent * (-1) + cleanedMantissa.length, '0');
            } else {
                finalResult = mantissaStr.substr(0, newExponent) + '.' + mantissaStr.substr(newExponent).replace(/0+$/, '');
            }
        }
        return (this.isNegative(xfl) ? '-' : '') + finalResult.replace(/\.+$/, '');
    }

    static getXfl(floatStr) {
        let exponent;
        let mantissa;
        if (floatStr.startsWith('.')) {
            exponent = BigInt(floatStr.length - 1);
            mantissa = BigInt(parseInt(floatStr.substr(1)));
        }
        else if (floatStr.endsWith('.')) {
            exponent = BigInt(0);
            mantissa = BigInt(parseInt(floatStr.substr(-1)));
        }
        else if (floatStr.includes('.')) {
            const parts = floatStr.split('.');
            exponent = BigInt(-parts[1].length);
            mantissa = BigInt(parseInt(parts.join('')));
        }
        else if (floatStr.endsWith('0')) {
            const mantissaStr = floatStr.replace(/0+$/g, "");
            exponent = BigInt(floatStr.length - mantissaStr.length);
            mantissa = BigInt(parseInt(mantissaStr));
        }
        else {
            exponent = BigInt(0);
            mantissa = BigInt(parseInt(floatStr));
        }

        // convert types as needed
        if (typeof (exponent) != 'bigint')
            exponent = BigInt(exponent);

        if (typeof (mantissa) != 'bigint')
            mantissa = BigInt(mantissa);

        // canonical zero
        if (mantissa == 0n)
            return 0n;

        // normalize
        let is_negative = mantissa < 0;
        if (is_negative)
            mantissa *= -1n;

        while (mantissa > maxMantissa) {
            mantissa /= 10n;
            exponent++;
        }
        while (mantissa < minMantissa) {
            mantissa *= 10n;
            exponent--;
        }

        // canonical zero on mantissa underflow
        if (mantissa == 0)
            return 0n;

        // under and overflows
        if (exponent > maxExponent || exponent < minExponent)
            return -1; // note this is an "invalid" XFL used to propagate errors

        exponent += 97n;

        let xfl = (is_negative ? 0n : 1n);
        xfl <<= 8n;
        xfl |= BigInt(exponent);
        xfl <<= 54n;
        xfl |= BigInt(mantissa);

        return xfl;
    }
}

module.exports = {
    XflHelpers
}