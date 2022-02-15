#ifndef STATEKEYS_INCLUDED
#define STATEKEYS_INCLUDED 1

/////////// Singelton keys. ///////////

// Host count (Maintains total no. of registered hosts)
// value 50 is in decimal. Its converted to 32 in hex.
const uint8_t STK_HOST_COUNT[32] = {'E', 'V', 'R', 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/////////// Repetitive state keys. ///////////

// Host id keys (Host registration entries for id-based lookup). Last 4 bytes will be replaced by host id in runtime.
uint8_t STP_HOST_ID[32] = {'E', 'V', 'R', 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Host address keys (Host registration entries for xrpl address-based lookup). Last 20 bytes will be replaced by host address in runtime.
uint8_t STP_HOST_ADDR[32] = {'E', 'V', 'R', 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/////////// Hook Configuration. ///////////
// All configuration keys has the prefix STP_CONF = 1;
// No. of Evers that will be ever issued.
const uint8_t CONF_MINT_LIMIT[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
// The host registration fee in Evers.
const uint8_t CONF_HOST_REG_FEE[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};
// Portion of registration fee forfeit by the foundation.
const uint8_t CONF_FIXED_REG_FEE[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3};
// No. of theoretical maximum registrants.
const uint8_t CONF_MAX_REG[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4};


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

#define HOST_ADDR_KEY_GUARD(host_addr, n)            \
    {                                                \
        for (int i = 12; GUARD(21 * n), i < 32; i++) \
            STP_HOST_ADDR[i] = host_addr[i - 12];    \
    }

#define HOST_ID_KEY_GUARD(host_id, n)               \
    {                                               \
        for (int i = 28; GUARD(5 * n), i < 32; i++) \
            STP_HOST_ID[i] = host_id[i - 28];       \
    }

#define CONF_KEY(buf, conf_key)                        \
    {                                                  \
        uint8_t *ptr = &conf_key;                      \
        STATE_KEY(buf, STP_CONF, ptr, sizeof(uint8_t)) \
    }

#endif