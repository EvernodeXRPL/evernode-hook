#ifndef STATEKEYS_INCLUDED
#define STATEKEYS_INCLUDED 1

/////////// Singelton keys. ///////////

// Host count (Maintains total no. of registered hosts)
// value 50 is in decimal. Its converted to 32 in hex.
const uint8_t STK_HOST_COUNT[32] = {'E', 'V', 'R', 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Ledger index when the CONF_MOMENT_SIZE last changed on.
const uint8_t STK_MOMENT_BASE_IDX[32] = {'E', 'V', 'R', 51, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// The host registration fee in Evers.
const uint8_t STK_HOST_REG_FEE[32] = {'E', 'V', 'R', 52, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// No. of theoretical maximum registrants.
const uint8_t STK_MAX_REG[32] = {'E', 'V', 'R', 53, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/////////// Repetitive state keys. ///////////

// Token id keys (Host registration entries for token id-based lookup).
uint8_t STP_TOKEN_ID[32] = {'E', 'V', 'R', 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Host address keys (Host registration entries for xrpl address-based lookup). Last 20 bytes will be replaced by host address in runtime.
uint8_t STP_HOST_ADDR[32] = {'E', 'V', 'R', 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/////////// Hook Configuration. ///////////
// All configuration keys has the prefix STP_CONF = 1;

// Issuer address.
const uint8_t CONF_ISSUER_ADDR[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
// Foundation address
const uint8_t CONF_FOUNDATION_ADDR[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};
// No. of ledgers per moment.
const uint8_t CONF_MOMENT_SIZE[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3};
// No. of Evers that will be ever issued.
const uint8_t CONF_MINT_LIMIT[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4};
// Portion of registration fee forfeit by the foundation.
const uint8_t CONF_FIXED_REG_FEE[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5};
// Moment frequency which host should keep signaling the registry contract (which used to track host aliveness).
const uint8_t CONF_HOST_HEARTBEAT_FREQ[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6};

const uint8_t CONF_PURCHASER_TARGET_PRICE[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7};
// Lease acquire ledger window.
const uint8_t CONF_LEASE_ACQUIRE_WINDOW[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8};

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

#define TOKEN_ID_KEY(token_id)                  \
    {                                           \
        for (int i = 4; GUARD(28), i < 32; i++) \
            STP_TOKEN_ID[i] = token_id[i];      \
    }

#define HOST_ADDR_KEY_GUARD(host_addr, n)            \
    {                                                \
        for (int i = 12; GUARD(21 * n), i < 32; i++) \
            STP_HOST_ADDR[i] = host_addr[i - 12];    \
    }

#define TOKEN_ID_KEY_GUARD(token_id, n)             \
    {                                               \
        for (int i = 4; GUARD(29 * n), i < 32; i++) \
            STP_TOKEN_ID[i] = token_id[i];          \
    }

#define CONF_KEY(buf, conf_key)                        \
    {                                                  \
        uint8_t *ptr = &conf_key;                      \
        STATE_KEY(buf, STP_CONF, ptr, sizeof(uint8_t)) \
    }

#endif