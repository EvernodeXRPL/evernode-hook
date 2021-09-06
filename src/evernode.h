#ifndef EVERNODE_INCLUDED
#define EVERNODE_INCLUDED 1

// Singelton keys.

// Host count (Maintains total no. of registered hosts)
uint8_t STK_HOST_COUNT[32] = {01, 01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Auditor count (Maintains total no. of registered auditors)
uint8_t STK_AUDITOR_COUNT[32] = {01, 02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Repetitive state keys.
uint8_t STP_CONF = 1;         // Configuration keys (Holds paramateres tunable by governance game)
uint8_t STP_HOST_ID = 2;      // Host id keys (Host registration entries for id-based lookup)
uint8_t STP_HOST_ADDR = 3;    // Host address keys (Host registration entries for xrpl address-based lookup)
uint8_t STP_AUDITOR_ID = 4;   // Auditor id keys (Auditor registration entries for id-based lookup)
uint8_t STP_AUDITOR_ADDR = 5; // Auditor address keys (Auditor registration entries for xrpl address-based lookup)
uint8_t STP_REDEEM_OP = 6;    // Redeem operation keys (Keys to hold ongoing redeem opration statuses)

// Hook Configuration.
uint8_t CONF_MOMENT_SIZE = 1;   // No. of ledgers per moment.
uint8_t CONF_MINT_LIMIT = 2;    // No. of Evers that will be ever issued.
uint8_t CONF_HOST_REG_FEE = 3;  // The host registration fee in Evers.
uint8_t CONF_MIN_REDEEM = 4;    // The minimum amount of hosting token spending allowed in a redeem operation.
uint8_t CONF_REDEEM_WINDOW = 5; // Max no. of ledgers within which a redeem operation has to be serviced.
uint8_t CONF_HOST_REWARD = 6;   // No. of Evers rewarded to a host when an audit passes.

// Default values.
uint16_t DEF_MOMENT_SIZE = 72;
uint64_t DEF_MINT_LIMIT = 25804800;
uint16_t DEF_HOST_REG_FEE = 5;
uint16_t DEF_MIN_REDEEM = 12;
uint16_t DEF_REDEEM_WINDOW = 12;
uint16_t DEF_HOST_REWARD = 1;

uint8_t currency[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'E', 'V', 'R', 0, 0, 0, 0, 0};

// macro to check currency + issuer at the same time
// truncate last bytes if key exceeds 32 bytes

#define IS_EVR(is_evr, amount_buffer, currency, issuer) \
    is_evr = 1;                                         \
    for (int i = 0; GUARD(20), i < 20; ++i)             \
    {                                                   \
        if (amount_buffer[i + 8] != currency[i] ||      \
            amount_buffer[i + 28] != issuer[i])         \
        {                                               \
            is_evr = 0;                                 \
            break;                                      \
        }                                               \
    }

#define STATE_KEY(buf, prefix, key, key_len)                  \
    buf[0] = 'E';                                             \
    buf[1] = 'V';                                             \
    buf[2] = 'R';                                             \
    buf[3] = prefix;                                          \
    {                                                         \
        int pad_len = sizeof(buf) - 4 - key_len;              \
        if (pad_len < 0)                                      \
            pad_len = 0;                                      \
        for (int i = 0; GUARDM(pad_len, 1), i < pad_len; i++) \
            buf[i + 4] = 0;                                   \
                                                              \
        for (int j = 0; GUARDM(key_len, 2), j < key_len; j++) \
            buf[j + pad_len + 4] = key[j];                    \
    }

#define CONF_KEY(buf, conf_key)                        \
    {                                                  \
        uint8_t *ptr = &conf_key;                      \
        STATE_KEY(buf, STP_CONF, ptr, sizeof(uint8_t)) \
    }

#define UINT32_TO_BYTES(buf, val)         \
    buf[0] = (uint8_t)(val >> 24) & 0xff; \
    buf[1] = (uint8_t)(val >> 16) & 0xff; \
    buf[2] = (uint8_t)(val >> 8) & 0xff;  \
    buf[3] = (uint8_t)(val >> 0) & 0xff;

#define BYTES_TO_UINT32(val, buf) val = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3] << 0);

#define ttTRUST_SET 20
#define tfClearNoRipple 0x00040000 // Disable the No Ripple flag, allowing rippling on this trust line.
#define PREPARE_SIMPLE_TRUSTLINE_SIZE 245
#define PREPARE_SIMPLE_TRUSTLINE(buf_out_master, tlamt, drops_fee_raw, to_address) \
    {                                                                              \
        uint8_t *buf_out = buf_out_master;                                         \
        uint8_t acc[20];                                                           \
        uint64_t drops_fee = (drops_fee_raw);                                      \
        uint32_t cls = (uint32_t)ledger_seq();                                     \
        hook_account(SBUF(acc));                                                   \
        _01_02_ENCODE_TT(buf_out, ttTRUST_SET);        /* uint16  | size   3 */    \
        _02_02_ENCODE_FLAGS(buf_out, tfClearNoRipple); /* uint32  | size   5 */    \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);            /* uint32  | size   5 */    \
        _02_26_ENCODE_FLS(buf_out, cls + 1);           /* uint32  | size   6 */    \
        _02_27_ENCODE_LLS(buf_out, cls + 5);           /* uint32  | size   6 */    \
        ENCODE_TL(buf_out, tlamt, amLIMITAMOUNT);      /* amount  | size  48 */    \
        _06_08_ENCODE_DROPS_FEE(buf_out, drops_fee);   /* amount  | size   9 */    \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);    /* pk      | size  35 */    \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);       /* account | size  22 */    \
        etxn_details((uint32_t)buf_out, 105);          /* emitdet | size 105 */    \
    }

#endif
