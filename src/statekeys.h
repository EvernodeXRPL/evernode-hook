#ifndef STATEKEYS_INCLUDED
#define STATEKEYS_INCLUDED 1

/////////// Singelton keys. ///////////

// Host count (Maintains total no. of registered hosts)
// value 50 is in decimal. Its converted to 32 in hex.
const uint8_t STK_HOST_COUNT[32] = {'E', 'V', 'R', 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Auditor count (Maintains total no. of registered auditors)
// value 51 is in decimal. Its converted to 33 in hex.
const uint8_t STK_AUDITOR_COUNT[32] = {'E', 'V', 'R', 51, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Ledger index when the CONF_MOMENT_SIZE last changed on
// value 52 is in decimal. Its converted to 34 in hex.
const uint8_t STK_MOMENT_BASE_IDX[32] = {'E', 'V', 'R', 52, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Moment start index and the moment seed(ledger hash) for the current moment
// value 53 is in decimal. Its converted to 35 in hex.
const uint8_t STK_MOMENT_SEED[32] = {'E', 'V', 'R', 53, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/////////// Repetitive state keys. ///////////

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

/////////// Hook Configuration. ///////////
// All configuration keys has the prefix STP_CONF = 1;
// Configuration keys (Holds paramateres tunable by governance game)
// No. of ledgers per moment.
const uint8_t CONF_MOMENT_SIZE[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
// No. of Evers that will be ever issued.
const uint8_t CONF_MINT_LIMIT[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};
// The host registration fee in Evers.
const uint8_t CONF_HOST_REG_FEE[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3};
// The minimum amount of hosting token spending allowed in a redeem operation.
const uint8_t CONF_MIN_REDEEM[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4};
// Max no. of ledgers within which a redeem operation has to be serviced.
const uint8_t CONF_REDEEM_WINDOW[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5};
// No. of Evers rewarded per moment.
const uint8_t CONF_REWARD[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6};
// No. of maximum hosts that can be rewarded per moment.
const uint8_t CONF_MAX_REWARD[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7};
// No. of maximum hosts that can be audited by a audit per moment.
const uint8_t CONF_MAX_AUDIT[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8};

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

#endif