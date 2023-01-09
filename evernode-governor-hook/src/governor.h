#ifndef GOVERNOR_INCLUDED
#define GOVERNOR_INCLUDED 1

#include "../../lib/hookapi.h"
#include "../../headers/evernode.h"
#include "../../headers/statekeys.h"
#include "../../headers/transactions.h"

// Parameters
// Hook address which contains the states
const uint8_t PARAM_STATE_HOOK[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

#define FOREIGN_REF 0, 0, 0, 0

///// Operational macros /////

// Equal operations

#define EQUAL_INITIALIZE(buf, len)                  \
    (sizeof(INITIALIZE) == (len + 1) &&             \
     BUFFER_EQUAL_8(buf, INITIALIZE) &&             \
     BUFFER_EQUAL_4((buf + 8), (INITIALIZE + 8)) && \
     BUFFER_EQUAL_1((buf + 12), (INITIALIZE + 12)))

// Copy operations

#define COPY_MOMENT_TRANSIT_INFO(lhsbuf, rhsbuf) \
    COPY_8BYTES(lhsbuf, rhsbuf);                 \
    COPY_2BYTES((lhsbuf + 8), (rhsbuf + 8));     \
    COPY_BYTE((lhsbuf + 10), (rhsbuf + 10));

// Clear operations

#define CLEAR_MOMENT_TRANSIT_INFO(buf) \
    CLEAR_8BYTES(buf);                 \
    CLEAR_2BYTES((buf + 8));           \
    CLEAR_BYTE((buf + 10))

// Empty check

#define IS_MOMENT_TRANSIT_INFO_EMPTY(buf) \
    (IS_BUFFER_EMPTY_8(buf) &&            \
     IS_BUFFER_EMPTY_2((buf + 8)) &&      \
     IS_BUFFER_EMPTY_1((buf + 10)))

// Domain operations

#define SET_UINT_STATE_VALUE(value, key, error_buf)                         \
    {                                                                       \
        uint8_t size = sizeof(value);                                       \
        uint8_t value_buf[size];                                            \
        switch (size)                                                       \
        {                                                                   \
        case 2:                                                             \
            UINT16_TO_BUF(value_buf, value);                                \
            break;                                                          \
        case 4:                                                             \
            UINT32_TO_BUF(value_buf, value);                                \
            break;                                                          \
        case 8:                                                             \
            UINT64_TO_BUF(value_buf, value);                                \
            break;                                                          \
        default:                                                            \
            rollback(SBUF("Evernode: Invalid state value set."), 1);        \
            break;                                                          \
        }                                                                   \
        if (state_foreign_set(SBUF(value_buf), SBUF(key), FOREIGN_REF) < 0) \
            rollback(SBUF(error_buf), 1);                                   \
    }

#endif
