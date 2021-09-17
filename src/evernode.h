#ifndef EVERNODE_INCLUDED
#define EVERNODE_INCLUDED 1

#define HOST_REG "evnHostReg"
#define REDEEM "evnRedeem"
#define REDEEM_REF "evnRedeemRef"
#define REDEEM_RESP "evnRedeemResp"
#define REFUND "evnRefund"
#define AUDIT_REQ "evnAuditRequest"

#define FORMAT_BINARY "binary"
#define FORMAT_TEXT "text/plain"

#define REDEEM_ERR "REDEEM_ERR"

// Singelton keys.

// Host count (Maintains total no. of registered hosts)
// value 50 is in decimal. Its converted to 32 in hex.
uint8_t STK_HOST_COUNT[32] = {'E', 'V', 'R', 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Auditor count (Maintains total no. of registered auditors)
// value 51 is in decimal. Its converted to 33 in hex.
uint8_t STK_AUDITOR_COUNT[32] = {'E', 'V', 'R', 51, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Ledger index when the CONF_MOMENT_SIZE last changed on
// value 52 is in decimal. Its converted to 34 in hex.
uint8_t STK_MOMENT_BASE_IDX[32] = {'E', 'V', 'R', 52, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Moment start index and the moment seed(ledger hash) for the current moment
// value 53 is in decimal. Its converted to 35 in hex.
uint8_t STK_MOMENT_SEED[32] = {'E', 'V', 'R', 53, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Repetitive state keys.

// Host id keys (Host registration entries for id-based lookup). Last 4 bytes will be replaced by host id in runtime.
uint8_t STP_HOST_ID[32] = {'E', 'V', 'R', 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Host address keys (Host registration entries for xrpl address-based lookup). Last 20 bytes will be replaced by host address in runtime.
uint8_t STP_HOST_ADDR[32] = {'E', 'V', 'R', 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Auditor id keys (Auditor registration entries for id-based lookup).
uint8_t STP_AUDITOR_ID[32] = {'E', 'V', 'R', 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Auditor address keys (Auditor registration entries for xrpl address-based lookup).
uint8_t STP_AUDITOR_ADDR[32] = {'E', 'V', 'R', 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Redeem operation keys (Redeem entries for hash-based lookup). Last 28 bytes will be replaced by tx hash in runtime.
uint8_t STP_REDEEM_OP[32] = {'E', 'V', 'R', 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Hook Configuration. All configuration keys has the prefix STP_CONF = 1;
// Configuration keys (Holds paramateres tunable by governance game)
// No. of ledgers per moment.
uint8_t CONF_MOMENT_SIZE[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
// No. of Evers that will be ever issued.
uint8_t CONF_MINT_LIMIT[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};
// The host registration fee in Evers.
uint8_t CONF_HOST_REG_FEE[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3};
// The minimum amount of hosting token spending allowed in a redeem operation.
uint8_t CONF_MIN_REDEEM[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4};
// Max no. of ledgers within which a redeem operation has to be serviced.
uint8_t CONF_REDEEM_WINDOW[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5};
// No. of Evers rewarded to a host when an audit passes.
uint8_t CONF_HOST_REWARD[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6};
// No. of No of maximum hosts that can be rewarded per moment.
uint8_t CONF_MAX_REWARD[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7};

// Default values.
uint16_t DEF_MOMENT_SIZE = 72;
uint64_t DEF_MINT_LIMIT = 25804800;
uint16_t DEF_HOST_REG_FEE = 5;
uint16_t DEF_MIN_REDEEM = 12;
uint16_t DEF_REDEEM_WINDOW = 12;
uint16_t DEF_HOST_REWARD = 1;
uint16_t DEF_MAX_REWARD = 20;
uint8_t DEF_AUDITOR_ADDR[35] = "rUWDtXPk4gAp8L6dNS51hLArnwFk4bRxky";    // This is a hard coded value, can be changed later.

uint8_t currency[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'E', 'V', 'R', 0, 0, 0, 0, 0};

// Constants
int32_t HOST_ADDR_VAL_SIZE = 43;
int32_t AUDITOR_ADDR_VAL_SIZE = 32;
int32_t REDEEM_STATE_VAL_SIZE = 39;
int32_t MOMENT_SEED_VAL_SIZE = 40;
int32_t AMOUNT_BUF_SIZE = 48;
int32_t HASH_SIZE = 32;

// Checks for EVR currency issued by hook account.
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

#define STATE_KEY(buf, prefix, key, key_len)                      \
    buf[0] = 'E';                                                 \
    buf[1] = 'V';                                                 \
    buf[2] = 'R';                                                 \
    buf[3] = prefix;                                              \
    {                                                             \
        int pad_len = sizeof(buf) - 4 - key_len;                  \
        int key_limit = key_len;                                  \
        if (pad_len < 0)                                          \
        {                                                         \
            pad_len = 0;                                          \
            key_limit = key_len - 4;                              \
        }                                                         \
        for (int i = 0; GUARDM(pad_len, 1), i < pad_len; i++)     \
            buf[i + 4] = 0;                                       \
                                                                  \
        for (int j = 0; GUARDM(key_limit, 2), j < key_limit; j++) \
            buf[j + pad_len + 4] = key[j];                        \
    }

#define HOST_ADDR_KEY(host_addr)                  \
    {                                             \
        for (int i = 12; GUARD(20), i < 32; i++)  \
            STP_HOST_ADDR[i] = host_addr[i - 12]; \
    }

#define HOST_ID_KEY(host_id)                    \
    {                                           \
        for (int i = 28; GUARD(4), i < 32; i++) \
            STP_HOST_ID[i] = host_id[i - 28];   \
    }

#define AUDITOR_ADDR_KEY(auditor_addr)                  \
    {                                                   \
        for (int i = 12; GUARD(20), i < 32; i++)        \
            STP_AUDITOR_ADDR[i] = auditor_addr[i - 12]; \
    }

#define AUDITOR_ID_KEY(auditor_id)                  \
    {                                               \
        for (int i = 28; GUARD(4), i < 32; i++)     \
            STP_AUDITOR_ID[i] = auditor_id[i - 28]; \
    }

#define REDEEM_OP_KEY(hash)                     \
    {                                           \
        for (int i = 4; GUARD(28), i < 32; i++) \
            STP_REDEEM_OP[i] = hash[i - 4];     \
    }

#define CONF_KEY(buf, conf_key)                        \
    {                                                  \
        uint8_t *ptr = &conf_key;                      \
        STATE_KEY(buf, STP_CONF, ptr, sizeof(uint8_t)) \
    }

#define ttCHECK_CREATE 16
#define ttCHECK_CASH 17
#define ttTRUST_SET 20

#define tfClearNoRipple 0x00040000 // Disable the No Ripple flag, allowing rippling on this trust line.

#define PREPARE_SIMPLE_TRUSTLINE_SIZE 245
#define PREPARE_SIMPLE_TRUSTLINE(buf_out_master, tlamt, drops_fee_raw)          \
    {                                                                           \
        uint8_t *buf_out = buf_out_master;                                      \
        uint8_t acc[20];                                                        \
        uint64_t drops_fee = (drops_fee_raw);                                   \
        uint32_t cls = (uint32_t)ledger_seq();                                  \
        hook_account(SBUF(acc));                                                \
        _01_02_ENCODE_TT(buf_out, ttTRUST_SET);        /* uint16  | size   3 */ \
        _02_02_ENCODE_FLAGS(buf_out, tfClearNoRipple); /* uint32  | size   5 */ \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);            /* uint32  | size   5 */ \
        _02_26_ENCODE_FLS(buf_out, cls + 1);           /* uint32  | size   6 */ \
        _02_27_ENCODE_LLS(buf_out, cls + 5);           /* uint32  | size   6 */ \
        ENCODE_TL(buf_out, tlamt, amLIMITAMOUNT);      /* amount  | size  48 */ \
        _06_08_ENCODE_DROPS_FEE(buf_out, drops_fee);   /* amount  | size   9 */ \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);    /* pk      | size  35 */ \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);       /* account | size  22 */ \
        etxn_details((uint32_t)buf_out, 105);          /* emitdet | size 105 */ \
    }

#define PREPARE_SIMPLE_CHECK_SIZE 261
#define PREPARE_SIMPLE_CHECK(buf_out_master, tlamt, drops_fee_raw, to_address)   \
    {                                                                            \
        uint8_t *buf_out = buf_out_master;                                       \
        uint8_t acc[20];                                                         \
        uint64_t drops_fee = (drops_fee_raw);                                    \
        uint32_t cls = (uint32_t)ledger_seq();                                   \
        hook_account(SBUF(acc));                                                 \
        _01_02_ENCODE_TT(buf_out, ttCHECK_CREATE);      /* uint16  | size   3 */ \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);             /* uint32  | size   5 */ \
        _02_26_ENCODE_FLS(buf_out, cls + 1);            /* uint32  | size   6 */ \
        _02_27_ENCODE_LLS(buf_out, cls + 5);            /* uint32  | size   6 */ \
        ENCODE_TL(buf_out, tlamt, amSENDMAX);           /* amount  | size  48 */ \
        _06_08_ENCODE_DROPS_FEE(buf_out, drops_fee);    /* amount  | size   9 */ \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);     /* pk      | size  35 */ \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);        /* account | size  22 */ \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address); /* account | size  22 */ \
        etxn_details((uint32_t)buf_out, 105);           /* emitdet | size 105 */ \
    }

#define ASCII_TO_HEX(val)    \
    {                        \
        switch (val)         \
        {                    \
        case 'A':            \
            val = 10;        \
            break;           \
        case 'B':            \
            val = 11;        \
            break;           \
        case 'C':            \
            val = 12;        \
            break;           \
        case 'D':            \
            val = 13;        \
            break;           \
        case 'E':            \
            val = 14;        \
            break;           \
        case 'F':            \
            val = 15;        \
            break;           \
        default:             \
            val = val - '0'; \
            break;           \
        }                    \
    }

#define HEXSTR_TO_BYTES(byte_ptr, hexstr_ptr, hexstr_len)          \
    {                                                              \
        for (int i = 0; GUARD(hexstr_len), i < hexstr_len; i += 2) \
        {                                                          \
            int val1 = (int)hexstr_ptr[i];                         \
            int val2 = (int)hexstr_ptr[i + 1];                     \
            ASCII_TO_HEX(val1)                                     \
            ASCII_TO_HEX(val2)                                     \
            byte_ptr[i / 2] = ((val1 * 16) + val2);                \
        }                                                          \
    }

#define IS_BUF_EMPTY(is_empty, buf, buflen)         \
    is_empty = 1;                                   \
    for (int i = 0; GUARD(buflen), i < buflen; ++i) \
    {                                               \
        if (buf[i] != 0)                            \
        {                                           \
            is_empty = 0;                           \
            break;                                  \
        }                                           \
    }

#endif
