#ifndef GOVERNOR_INCLUDED
#define GOVERNOR_INCLUDED 1

#include "../../lib/hookapi.h"
#include "../../headers/evernode.h"
#include "../../headers/statekeys.h"

#define PILOTED_MODE_CAND_SHORTNAME "piloted_mode"
#define DUD_HOST_CAND_SHORTNAME "dud_host"

#define OP_INITIALIZE 1
#define OP_PROPOSE 2
#define OP_GOVERNANCE_MODE_CHANGE 3
#define OP_STATUS_CHANGE 4
#define OP_WITHDRAW 5
#define OP_DUD_HOST_REPORT 6
#define OP_REMOVE_LINKED_CANDIDATE 7
#define OP_REMOVE 8

#define FOREIGN_REF 0, 0, 0, 0

#define EQUAL_INITIALIZE(buf, len)                  \
    (sizeof(INITIALIZE) == (len + 1) &&             \
     BUFFER_EQUAL_8(buf, INITIALIZE) &&             \
     BUFFER_EQUAL_4((buf + 8), (INITIALIZE + 8)) && \
     BUFFER_EQUAL_1((buf + 12), (INITIALIZE + 12)))

#define EQUAL_CANDIDATE_PROPOSE(buf, len)                \
    (sizeof(CANDIDATE_PROPOSE) == (len + 1) &&           \
     BUFFER_EQUAL_8(buf, CANDIDATE_PROPOSE) &&           \
     BUFFER_EQUAL_8(buf + 8, CANDIDATE_PROPOSE + 8) &&   \
     BUFFER_EQUAL_2(buf + 16, CANDIDATE_PROPOSE + 16) && \
     BUFFER_EQUAL_1((buf + 18), (CANDIDATE_PROPOSE + 18)))

#define EQUAL_CANDIDATE_PROPOSE_REF(buf, len)                    \
    (sizeof(CANDIDATE_PROPOSE_REF) == (len + 1) &&               \
     BUFFER_EQUAL_8(buf, CANDIDATE_PROPOSE_REF) &&               \
     BUFFER_EQUAL_8((buf + 8), (CANDIDATE_PROPOSE_REF + 8)) &&   \
     BUFFER_EQUAL_4((buf + 16), (CANDIDATE_PROPOSE_REF + 16)) && \
     BUFFER_EQUAL_2((buf + 20), (CANDIDATE_PROPOSE_REF + 20)))

#define EQUAL_CANDIDATE_STATUS_CHANGE(buf, len)                  \
    (sizeof(CANDIDATE_STATUS_CHANGE) == (len + 1) &&             \
     BUFFER_EQUAL_8(buf, CANDIDATE_STATUS_CHANGE) &&             \
     BUFFER_EQUAL_8((buf + 8), (CANDIDATE_STATUS_CHANGE + 8)) && \
     BUFFER_EQUAL_8((buf + 16), (CANDIDATE_STATUS_CHANGE + 16)))

#define EQUAL_CANDIDATE_WITHDRAW(buf, len)                  \
    (sizeof(CANDIDATE_WITHDRAW) == (len + 1) &&             \
     BUFFER_EQUAL_8(buf, CANDIDATE_WITHDRAW) &&             \
     BUFFER_EQUAL_8((buf + 8), (CANDIDATE_WITHDRAW + 8)) && \
     BUFFER_EQUAL_4((buf + 16), (CANDIDATE_WITHDRAW + 16)))

#define EQUAL_CHANGE_GOVERNANCE_MODE(buf, len)                    \
    (sizeof(CHANGE_GOVERNANCE_MODE) == (len + 1) &&               \
     BUFFER_EQUAL_8(buf, CHANGE_GOVERNANCE_MODE) &&               \
     BUFFER_EQUAL_8((buf + 8), (CHANGE_GOVERNANCE_MODE + 8)) &&   \
     BUFFER_EQUAL_4((buf + 16), (CHANGE_GOVERNANCE_MODE + 16)) && \
     BUFFER_EQUAL_2((buf + 20), (CHANGE_GOVERNANCE_MODE + 20)) && \
     BUFFER_EQUAL_1((buf + 22), (CHANGE_GOVERNANCE_MODE + 22)))

#define EQUAL_HOOK_UPDATE_RES(buf, len)      \
    (sizeof(HOOK_UPDATE_RES) == (len + 1) && \
     BUFFER_EQUAL_8(buf, HOOK_UPDATE_RES) && \
     BUFFER_EQUAL_8((buf + 8), (HOOK_UPDATE_RES + 8)))

#define EQUAL_DUD_HOST_REPORT(buf, len)      \
    (sizeof(DUD_HOST_REPORT) == (len + 1) && \
     BUFFER_EQUAL_8(buf, DUD_HOST_REPORT) && \
     BUFFER_EQUAL_8((buf + 8), (DUD_HOST_REPORT + 8)))

#define EQUAL_LINKED_CANDIDATE_REMOVE(buf, len)                         \
    (sizeof(LINKED_CANDIDATE_REMOVE) == (len + 1) &&             \
     BUFFER_EQUAL_8(buf, LINKED_CANDIDATE_REMOVE) &&             \
     BUFFER_EQUAL_8((buf + 8), (LINKED_CANDIDATE_REMOVE + 8)) && \
     BUFFER_EQUAL_8((buf + 16), (LINKED_CANDIDATE_REMOVE + 16)))

#define COPY_CANDIDATE_HASHES(lhsbuf, rhsbuf)   \
    COPY_32BYTES(lhsbuf, rhsbuf);               \
    COPY_32BYTES((lhsbuf + 32), (rhsbuf + 32)); \
    COPY_32BYTES((lhsbuf + 64), (rhsbuf + 64));

// Default values.
const uint8_t HOOK_INITIALIZER_ADDR[35] = "rMhUhCXnqk4wfeqNh7YBa89YtQGiRaimwZ";
const uint16_t DEF_MOMENT_SIZE = 120;
const uint16_t DEF_MOMENT_TYPE = TIMESTAMP_MOMENT_TYPE;
const uint64_t DEF_MINT_LIMIT = 72253440;
const uint64_t DEF_HOST_REG_FEE = 5120;
const uint64_t DEF_FIXED_REG_FEE = 5;
const uint64_t DEF_MAX_REG = 14112; // No. of theoretical maximum registrants. (72253440/5120)
const uint16_t DEF_HOST_HEARTBEAT_FREQ = 1;
const uint16_t DEF_LEASE_ACQUIRE_WINDOW = 160;   // In seconds
const uint16_t DEF_MAX_TOLERABLE_DOWNTIME = 240; // In moments.
const uint8_t DEF_EPOCH_COUNT = 10;
const uint32_t DEF_FIRST_EPOCH_REWARD_QUOTA = 5120;
const uint32_t DEF_EPOCH_REWARD_AMOUNT = 5160960;
const uint32_t DEF_REWARD_START_MOMENT = 0;
const int64_t DEF_EMIT_FEE_THRESHOLD = 1000;                // In Drops.
const uint32_t DEF_GOVERNANCE_ELIGIBILITY_PERIOD = 7884000; // 3 months in seconds.
const uint32_t DEF_CANDIDATE_LIFE_PERIOD = 7884000;         // 3 months in seconds.
const uint32_t DEF_CANDIDATE_ELECTION_PERIOD = 1209600;     // 2 weeks in seconds.
const uint16_t DEF_CANDIDATE_SUPPORT_AVERAGE = 80;
const uint16_t DEF_ACCUMULATED_REWARD_FREQUENCY = 24;

// Transition related definitions. Transition state is added on the init transaction if this has >0 value
const uint16_t NEW_MOMENT_SIZE = 3600;
const uint8_t NEW_MOMENT_TYPE = TIMESTAMP_MOMENT_TYPE;

// Domain related clear macros.

#define CLEAR_MOMENT_TRANSIT_INFO(buf) \
    CLEAR_8BYTES(buf);                 \
    CLEAR_2BYTES((buf + 8));           \
    CLEAR_BYTE((buf + 10))

// Domain related empty check macros.

#define IS_MOMENT_TRANSIT_INFO_EMPTY(buf) \
    (IS_BUFFER_EMPTY_8(buf) &&            \
     IS_BUFFER_EMPTY_2((buf + 8)) &&      \
     IS_BUFFER_EMPTY_1((buf + 10)))

#define IS_HOOKS_VALID(hook_keylets_ptr, hooks_exists)              \
    {                                                               \
        hooks_exists = 0;                                           \
        int64_t hook_slot = slot_set(hook_keylets_ptr, 34, 0);      \
        if (hook_slot >= 0)                                         \
        {                                                           \
            hook_slot = slot_set(hook_keylets_ptr + 34, 34, 0);     \
            if (hook_slot >= 0)                                     \
            {                                                       \
                hook_slot = slot_set(hook_keylets_ptr + 68, 34, 0); \
                if (hook_slot >= 0)                                 \
                    hooks_exists = 1;                               \
            }                                                       \
        }                                                           \
    }

#define CHECK_RUNNING_HOOK(account, hook_hash, is_valid)                    \
    {                                                                       \
        uint8_t keylet[34] = {0};                                           \
        UINT16_TO_BUF(keylet, ltHOOK);                                      \
                                                                            \
        uint8_t index_data[22] = {0};                                       \
        UINT16_TO_BUF(index_data, ltHOOK);                                  \
        COPY_20BYTES(&index_data[2], account);                              \
                                                                            \
        uint8_t index[HASH_SIZE];                                           \
        if (util_sha512h(SBUF(index), SBUF(index_data)) < 0)                \
            rollback(SBUF("Evernode: Could not generate hook keylet."), 1); \
        COPY_32BYTES(&keylet[2], index);                                    \
                                                                            \
        is_valid = 0;                                                       \
        int64_t cur_slot = slot_set(SBUF(keylet), 0);                       \
        if (cur_slot >= 0)                                                  \
        {                                                                   \
            cur_slot = slot_subfield(cur_slot, sfHooks, 0);                 \
            if (cur_slot >= 0)                                              \
            {                                                               \
                cur_slot = slot_subarray(cur_slot, 0, 0);                   \
                if (cur_slot >= 0)                                          \
                {                                                           \
                    uint8_t cur_hash[32] = {0};                             \
                    cur_slot = slot_subfield(cur_slot, sfHookHash, 0);      \
                    slot(SBUF(cur_hash), cur_slot);                         \
                    is_valid = BUFFER_EQUAL_32(cur_hash, hook_hash);        \
                }                                                           \
            }                                                               \
        }                                                                   \
    }

/**************************************************************************/
/*************Pre-populated templates of Payment Transactions**************/
/**************************************************************************/

// Simple XRP Payment with single memo.
uint8_t CANDIDATE_REBATE_MIN_PAYMENT[386] = {
    0x12, 0x00, 0x00,                                     // transaction_type(ttPAYMENT)
    0x22, 0x80, 0x00, 0x00, 0x00,                         // flags(tfCANONICAL)
    0x23, 0x00, 0x00, 0x00, 0x00,                         // TAG_SOURCE
    0x24, 0x00, 0x00, 0x00, 0x00,                         // sequence(0)
    0x2E, 0x00, 0x00, 0x00, 0x00,                         // TAG DESTINATION
    0x20, 0x1A, 0x00, 0x00, 0x00, 0x00,                   // first_ledger_sequence(ledger_seq + 1) - Added on prepare to offset 25
    0x20, 0x1B, 0x00, 0x00, 0x00, 0x00,                   // last_ledger_sequence(ledger_seq + 5) - Added on prepare to offset 31
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // amount(<type(1)><amount(8)>) - Added on prepare to offset 35
    0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // fee Added on prepare to offset 44
    0x73, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Signing Public Key (NULL offset 55)
    0x81, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_source(20) - Added on prepare to offset 90
    0x83, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_destination(20) - Added on prepare to offset 112
    0xF0, 0x13, // Hook parameter array start marker
    0xE0, 0x17, // Hook parameter object start marker
    0x70,       // Blob start marker
    0x18, 0x20, // Parameter name length 32 bytes
    0x45, 0x56, 0x52, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, // Parameter name
    0x70,                                                                   // Blob start marker
    0x19, 0x15,                                                             // Parameter value length 21 bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,       // Event Type (21 bytes) offset 174
    0xE1,       // Hook parameter object end marker
    0xE0, 0x17, // Hook parameter object start marker
    0x70,       // Blob start marker
    0x18, 0x20, // Parameter name length 32 bytes
    0x45, 0x56, 0x52, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, // Parameter name
    0x70,                                                                   // Blob start marker
    0x19, 0x20,                                                             // Parameter value length 32 bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Event Data (32 bytes) offset 236
    0xE1,                                                                   // Hook parameter object end marker
    0xF1,                                                                   // Hook parameter array end marker
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit_details(116) - Added on prepare to offset 270
};

#define CANDIDATE_REBATE_COMMON(buf_out, to_address, param_1, param_2) \
    {                                                                  \
        COPY_20BYTES((buf_out), hook_accid);                           \
        COPY_20BYTES((buf_out + 22), to_address);                      \
        COPY_8BYTES((buf_out + 84), param_1);                          \
        COPY_8BYTES((buf_out + 84 + 8), (param_1 + 8));                \
        COPY_4BYTES((buf_out + 84 + 16), (param_1 + 16));              \
        COPY_BYTE((buf_out + 84 + 20), (param_1 + 20));                \
        COPY_32BYTES((buf_out + 146), param_2);                        \
    }

#define CANDIDATE_REBATE_MIN_PAYMENT_TX_SIZE \
    sizeof(CANDIDATE_REBATE_MIN_PAYMENT)
#define PREPARE_CANDIDATE_REBATE_MIN_PAYMENT_TX(drops_amount, to_address, param_1, param_2) \
    {                                                                                       \
        uint8_t *buf_out = CANDIDATE_REBATE_MIN_PAYMENT;                                    \
        UINT32_TO_BUF((buf_out + 25), cur_ledger_seq + 1);                                  \
        UINT32_TO_BUF((buf_out + 31), cur_ledger_seq + 5);                                  \
        uint8_t *buf_ptr = (buf_out + 35);                                                  \
        _06_01_ENCODE_DROPS_AMOUNT(buf_ptr, drops_amount);                                  \
        CANDIDATE_REBATE_COMMON((buf_out + 90), to_address, param_1, param_2);              \
        etxn_details((buf_out + 270), 116);                                                 \
        int64_t fee = etxn_fee_base(buf_out, CANDIDATE_REBATE_MIN_PAYMENT_TX_SIZE);         \
        buf_ptr = buf_out + 44;                                                             \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(buf_ptr, fee);                                       \
    }

// IOU Payment with single memo
uint8_t CANDIDATE_REBATE_PAYMENT[426] = {
    0x12, 0x00, 0x00,                   // transaction_type(ttPAYMENT)
    0x22, 0x80, 0x00, 0x00, 0x00,       // flags(tfCANONICAL)
    0x23, 0x00, 0x00, 0x00, 0x00,       // TAG_SOURCE
    0x24, 0x00, 0x00, 0x00, 0x00,       // sequence(0)
    0x2E, 0x00, 0x00, 0x00, 0x00,       // TAG DESTINATION
    0x20, 0x1A, 0x00, 0x00, 0x00, 0x00, // first_ledger_sequence(ledger_seq + 1) - Added on prepare to offset 25
    0x20, 0x1B, 0x00, 0x00, 0x00, 0x00, // last_ledger_sequence(ledger_seq + 5) - Added on prepare to offset 31
    0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // amount(<type(1)><amount(8)><currency_code(20)><issuer(20)>) - Added on prepare to offset 35
    0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // fee - Added on prepare to offset 84
    0x73, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Signing Public Key (NULL offset 95)
    0x81, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_source(20) - Added on prepare to offset 130
    0x83, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_destination(20) - Added on prepare to offset 152
    0xF0, 0x13, // Hook parameter array start marker
    0xE0, 0x17, // Hook parameter object start marker
    0x70,       // Blob start marker
    0x18, 0x20, // Parameter name length 32 bytes
    0x45, 0x56, 0x52, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, // Parameter name
    0x70,                                                                   // Blob start marker
    0x19, 0x15,                                                             // Parameter value length 21 bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,       // Event Type (21 bytes) offset 214
    0xE1,       // Hook parameter object end marker
    0xE0, 0x17, // Hook parameter object start marker
    0x70,       // Blob start marker
    0x18, 0x20, // Parameter name length 32 bytes
    0x45, 0x56, 0x52, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, // Parameter name
    0x70,                                                                   // Blob start marker
    0x19, 0x20,                                                             // Parameter value length 32 bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Event Data (32 bytes) offset 276
    0xE1,                                                                   // Hook parameter object end marker
    0xF1,                                                                   // Hook parameter array end marker
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit_details(116) - Added on prepare to offset 310
};

#define CANDIDATE_REBATE_PAYMENT_TX_SIZE \
    sizeof(CANDIDATE_REBATE_PAYMENT)
#define PREPARE_CANDIDATE_REBATE_PAYMENT_TX(evr_amount, to_address, param_1, param_2) \
    {                                                                                 \
        uint8_t *buf_out = CANDIDATE_REBATE_PAYMENT;                                  \
        UINT32_TO_BUF((buf_out + 25), cur_ledger_seq + 1);                            \
        UINT32_TO_BUF((buf_out + 31), cur_ledger_seq + 5);                            \
        SET_AMOUNT_OUT((buf_out + 35), EVR_TOKEN, issuer_accid, evr_amount);          \
        CANDIDATE_REBATE_COMMON((buf_out + 130), to_address, param_1, param_2);       \
        etxn_details((buf_out + 310), 116);                                           \
        int64_t fee = etxn_fee_base(buf_out, CANDIDATE_REBATE_PAYMENT_TX_SIZE);       \
        uint8_t *fee_ptr = buf_out + 84;                                              \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee);                                 \
    }

// Simple XRP Payment with single memo.
uint8_t HOOK_UPDATE_PAYMENT[378] = {
    0x12, 0x00, 0x00,                                     // transaction_type(ttPAYMENT)
    0x22, 0x80, 0x00, 0x00, 0x00,                         // flags(tfCANONICAL)
    0x23, 0x00, 0x00, 0x00, 0x00,                         // TAG_SOURCE
    0x24, 0x00, 0x00, 0x00, 0x00,                         // sequence(0)
    0x2E, 0x00, 0x00, 0x00, 0x00,                         // TAG DESTINATION
    0x20, 0x1A, 0x00, 0x00, 0x00, 0x00,                   // first_ledger_sequence(ledger_seq + 1) - Added on prepare to offset 25
    0x20, 0x1B, 0x00, 0x00, 0x00, 0x00,                   // last_ledger_sequence(ledger_seq + 5) - Added on prepare to offset 31
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // amount(<type(1)><amount(8)>) - Added on prepare to offset 35
    0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // fee Added on prepare to offset 44
    0x73, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Signing Public Key (NULL offset 55)
    0x81, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_source(20) - Added on prepare to offset 90
    0x83, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_destination(20) - Added on prepare to offset 112
    0xF0, 0x13, // Hook parameter array start marker
    0xE0, 0x17, // Hook parameter object start marker
    0x70,       // Blob start marker
    0x18, 0x20, // Parameter name length 32 bytes
    0x45, 0x56, 0x52, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,       // Parameter name
    0x70,                                                                         // Blob start marker
    0x19, 0x0D,                                                                   // Parameter value length 13 bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Event Type (13 bytes) offset 174
    0xE1,                                                                         // Hook parameter object end marker
    0xE0, 0x17,                                                                   // Hook parameter object start marker
    0x70,                                                                         // Blob start marker
    0x18, 0x20,                                                                   // Parameter name length 32 bytes
    0x45, 0x56, 0x52, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, // Parameter name
    0x70,                                                                   // Blob start marker
    0x19, 0x20,                                                             // Parameter value length 32 bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Event Data (32 bytes) offset 228
    0xE1,                                                                   // Hook parameter object end marker
    0xF1,                                                                   // Hook parameter array end marker
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit_details(116) - Added on prepare to offset 262
};

#define HOOK_UPDATE_PAYMENT_TX_SIZE \
    sizeof(HOOK_UPDATE_PAYMENT)
#define PREPARE_HOOK_UPDATE_PAYMENT_TX(drops_amount, to_address, unique_id)                                              \
    {                                                                                                                    \
        uint8_t *buf_out = HOOK_UPDATE_PAYMENT;                                                                          \
        UINT32_TO_BUF((buf_out + 25), cur_ledger_seq + 1);                                                               \
        UINT32_TO_BUF((buf_out + 31), cur_ledger_seq + 5);                                                               \
        uint8_t *buf_ptr = (buf_out + 35);                                                                               \
        _06_01_ENCODE_DROPS_AMOUNT(buf_ptr, drops_amount);                                                               \
        COPY_20BYTES((buf_out + 90), hook_accid);                                                                        \
        COPY_20BYTES((buf_out + 112), to_address);                                                                       \
        COPY_8BYTES((buf_out + 174), HOOK_UPDATE);                                                                       \
        COPY_4BYTES((buf_out + 174 + 8), HOOK_UPDATE + 8);                                                               \
        COPY_BYTE((buf_out + 174 + 12), HOOK_UPDATE + 12);                                                               \
        COPY_32BYTES((buf_out + 228), unique_id);                                                                        \
        etxn_details((buf_out + 262), 116);                                                                              \
        int64_t fee = etxn_fee_base(buf_out, HOOK_UPDATE_PAYMENT_TX_SIZE);                                               \
        buf_ptr = buf_out + 44;                                                                                          \
        _06_08_ENCODE_DROPS_FEE(buf_ptr, fee); /** Skip the fee check since this tx is sent to registry/governor hook.*/ \
    }

// Simple XRP Payment with single memo.
uint8_t DUD_HOST_REMOVE_TX[369] = {
    0x12, 0x00, 0x00,                                     // transaction_type(ttPAYMENT)
    0x22, 0x80, 0x00, 0x00, 0x00,                         // flags(tfCANONICAL)
    0x23, 0x00, 0x00, 0x00, 0x00,                         // TAG_SOURCE
    0x24, 0x00, 0x00, 0x00, 0x00,                         // sequence(0)
    0x2E, 0x00, 0x00, 0x00, 0x00,                         // TAG DESTINATION
    0x20, 0x1A, 0x00, 0x00, 0x00, 0x00,                   // first_ledger_sequence(ledger_seq + 1) - Added on prepare to offset 25
    0x20, 0x1B, 0x00, 0x00, 0x00, 0x00,                   // last_ledger_sequence(ledger_seq + 5) - Added on prepare to offset 31
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // amount(<type(1)><amount(8)>) - Added on prepare to offset 35
    0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // fee Added on prepare to offset 44
    0x73, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Signing Public Key (NULL offset 55)
    0x81, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_source(20) - Added on prepare to offset 90
    0x83, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_destination(20) - Added on prepare to offset 112
    0xF0, 0x13, // Hook parameter array start marker
    0xE0, 0x17, // Hook parameter object start marker
    0x70,       // Blob start marker
    0x18, 0x20, // Parameter name length 32 bytes
    0x45, 0x56, 0x52, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,                         // Parameter name
    0x70,                                                                                           // Blob start marker
    0x19, 0x10,                                                                                     // Parameter value length 16 bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Event Type (16 bytes) offset 174
    0xE1,                                                                                           // Hook parameter object end marker
    0xE0, 0x17,                                                                                     // Hook parameter object start marker
    0x70,                                                                                           // Blob start marker
    0x18, 0x20,                                                                                     // Parameter name length 32 bytes
    0x45, 0x56, 0x52, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,                                                 // Parameter name
    0x70,                                                                                                                   // Blob start marker
    0x19, 0x14,                                                                                                             // Parameter value length 20 bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Event Data (20 bytes) offset 231
    0xE1,                                                                                                                   // Hook parameter object end marker
    0xF1,                                                                                                                   // Hook parameter array end marker
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit_details(116) - Added on prepare to offset 253
};

#define DUD_HOST_REMOVE_TX_SIZE \
    sizeof(DUD_HOST_REMOVE_TX)
#define PREPARE_DUD_HOST_REMOVE_TX(drops_amount, to_address, param_1, param_2) \
    {                                                                          \
        uint8_t *buf_out = DUD_HOST_REMOVE_TX;                                 \
        UINT32_TO_BUF((buf_out + 25), cur_ledger_seq + 1);                     \
        UINT32_TO_BUF((buf_out + 31), cur_ledger_seq + 5);                     \
        uint8_t *buf_ptr = (buf_out + 35);                                     \
        _06_01_ENCODE_DROPS_AMOUNT(buf_ptr, drops_amount);                     \
        COPY_20BYTES((buf_out + 90), hook_accid);                              \
        COPY_20BYTES((buf_out + 112), to_address);                             \
        COPY_16BYTES((buf_out + 174), param_1);                                \
        COPY_20BYTES((buf_out + 231), param_2);                                \
        etxn_details((buf_out + 253), 116);                                    \
        int64_t fee = etxn_fee_base(buf_out, DUD_HOST_REMOVE_TX_SIZE);         \
        buf_ptr = buf_out + 44;                                                \
        _06_08_ENCODE_DROPS_FEE(buf_ptr, fee);                                 \
    }
#endif
