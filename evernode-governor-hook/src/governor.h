#ifndef GOVERNOR_INCLUDED
#define GOVERNOR_INCLUDED 1

#include "../../lib/hookapi.h"
#include "../../headers/evernode.h"
#include "../../headers/statekeys.h"

#define PILOTED_MODE_CAND_SHORTNAME "piloted_mode"

#define OP_INITIALIZE 1
#define OP_PROPOSE 2
#define OP_VOTE 3
#define OP_GOVERNANCE_MODE_CHANGE 4
#define OP_STATUS_CHANGE 5
#define OP_WITHDRAW 6
#define OP_DUD_HOST_REPORT 7

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

#define COPY_CANDIDATE_HASHES(lhsbuf, rhsbuf)   \
    COPY_32BYTES(lhsbuf, rhsbuf);               \
    COPY_32BYTES((lhsbuf + 32), (rhsbuf + 32)); \
    COPY_32BYTES((lhsbuf + 64), (rhsbuf + 64));

// Default values.
const uint8_t HOOK_INITIALIZER_ADDR[35] = "rEeFk3SpyCtt8mvjMgaAsvceHHh4nroezM";
const uint16_t DEF_MOMENT_SIZE = 3600;
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
const uint16_t DEF_CANDIDATE_ELECTION_PERIOD = 1209600;     // 2 weeks in seconds.
const uint16_t DEF_CANDIDATE_SUPPORT_AVERAGE = 80;

// Transition related definitions. Transition state is added on the init transaction if this has >0 value
const uint16_t NEW_MOMENT_SIZE = 3600;
const uint8_t NEW_MOMENT_TYPE = TIMESTAMP_MOMENT_TYPE;

#define SET_UINT_STATE_VALUE(value, key, error_buf)                  \
    {                                                                \
        uint8_t size = sizeof(value);                                \
        uint8_t value_buf[size];                                     \
        switch (size)                                                \
        {                                                            \
        case 2:                                                      \
            UINT16_TO_BUF(value_buf, value);                         \
            break;                                                   \
        case 4:                                                      \
            UINT32_TO_BUF(value_buf, value);                         \
            break;                                                   \
        case 8:                                                      \
            UINT64_TO_BUF(value_buf, value);                         \
            break;                                                   \
        default:                                                     \
            rollback(SBUF("Evernode: Invalid state value set."), 1); \
            break;                                                   \
        }                                                            \
        if (state_set(SBUF(value_buf), SBUF(key)) < 0)               \
            rollback(SBUF(error_buf), 1);                            \
    }

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

#define GET_NEW_HOOK_CANDIDATE_ID(hash_ptr, hash_len, id)                      \
    {                                                                          \
        if (util_sha512h(SBUF(id), hash_ptr, hash_len) < 0)                    \
            rollback(SBUF("Evernode: Could not generate candidate hash."), 1); \
        CLEAR_4BYTES(id);                                                      \
        *(uint8_t *)(id + 4) = (uint8_t)(NEW_HOOK_CANDIDATE);                  \
    }

#define GET_PILOTED_MODE_CANDIDATE_ID(id)                         \
    {                                                             \
        CLEAR_4BYTES(id);                                         \
        *(uint8_t *)(id + 4) = (uint8_t)(PILOTED_MODE_CANDIDATE); \
        COPY_16BYTES(id + 5, NAMESPACE + 5);                      \
        COPY_8BYTES(id + 21, NAMESPACE + 21);                     \
        COPY_2BYTES(id + 29, NAMESPACE + 29);                     \
        COPY_BYTE(id + 31, NAMESPACE + 31);                       \
    }

#define GET_DUD_HOST_CANDIDATE_ID(host_account, id)           \
    {                                                         \
        CLEAR_4BYTES(id);                                     \
        *(uint8_t *)(id + 4) = (uint8_t)(DUD_HOST_CANDIDATE); \
        CLEAR_4BYTES(id + 5);                                 \
        CLEAR_2BYTES(id + 9);                                 \
        CLEAR_BYTE(id + 11);                                  \
        COPY_20BYTES(id + 12, host_account);                  \
    }

/**************************************************************************/
/*************Pre-populated templates of Payment Transactions**************/
/**************************************************************************/

// Simple XRP Payment with single memo.
uint8_t CANDIDATE_REBATE_MIN_PAYMENT[336] = {
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
    0xF9, 0xEA, // Memo array and object start markers
    0x7C, 0x15,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, // MemoType (21 bytes) offset 136
    0x7D, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MemoData (32 bytes) offset 159
    0x7E, 0x03,
    0x00, 0x00, 0x00, // MemoFormat (3 bytes) offset 193
    0xE1, 0xF1,       // Memo array and object end markers
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit_details(138) - Added on prepare to offset 198
    // emit_details - NOTE : Considered additional 22 bytes for the callback scenario.
};

#define CANDIDATE_REBATE_COMMON(buf_out, to_address, memo_type, memo_data, memo_format) \
    {                                                                                   \
        COPY_20BYTES((buf_out), hook_accid);                                            \
        COPY_20BYTES((buf_out + 22), to_address);                                       \
        COPY_8BYTES((buf_out + 46), memo_type);                                         \
        COPY_8BYTES((buf_out + 46 + 8), (memo_type + 8));                               \
        COPY_4BYTES((buf_out + 46 + 16), (memo_type + 16));                             \
        COPY_BYTE((buf_out + 46 + 20), (memo_type + 20));                               \
        COPY_32BYTES((buf_out + 69), memo_data);                                        \
        COPY_2BYTES((buf_out + 103), memo_format);                                      \
        COPY_BYTE((buf_out + 103 + 2), (memo_format + 2));                              \
    }

#define CANDIDATE_REBATE_MIN_PAYMENT_TX_SIZE \
    sizeof(CANDIDATE_REBATE_MIN_PAYMENT)
#define PREPARE_CANDIDATE_REBATE_MIN_PAYMENT_TX(drops_amount, to_address, memo_type, memo_data, memo_format) \
    {                                                                                                        \
        uint8_t *buf_out = CANDIDATE_REBATE_MIN_PAYMENT;                                                     \
        UINT32_TO_BUF((buf_out + 25), cur_ledger_seq + 1);                                                   \
        UINT32_TO_BUF((buf_out + 31), cur_ledger_seq + 5);                                                   \
        uint8_t *buf_ptr = (buf_out + 35);                                                                   \
        _06_01_ENCODE_DROPS_AMOUNT(buf_ptr, drops_amount);                                                   \
        CANDIDATE_REBATE_COMMON((buf_out + 90), to_address, memo_type, memo_data, memo_format);              \
        etxn_details((buf_out + 198), CANDIDATE_REBATE_MIN_PAYMENT_TX_SIZE);                                 \
        int64_t fee = etxn_fee_base(buf_out, CANDIDATE_REBATE_MIN_PAYMENT_TX_SIZE);                          \
        buf_ptr = buf_out + 44;                                                                              \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(buf_ptr, fee);                                                        \
    }

// IOU Payment with single memo
uint8_t CANDIDATE_REBATE_PAYMENT[376] = {
    0x12, 0x00, 0x00,                   // transaction_type(ttPAYMENT)
    0x22, 0x80, 0x00, 0x00, 0x00,       // flags(tfCANONICAL)
    0x23, 0x00, 0x00, 0x00, 0x00,       // TAG_SOURCE
    0x24, 0x00, 0x00, 0x00, 0x00,       // sequence(0)
    0x2E, 0x00, 0x00, 0x00, 0x00,       // TAG DESTINATION
    0x20, 0x1A, 0x00, 0x00, 0x00, 0x00, // first_ledger_sequence(ledger_seq + 1) - Added on prepare to offset 25
    0x20, 0x1B, 0x00, 0x00, 0x00, 0x00, // last_ledger_sequence(ledger_seq + 5) - Added on prepare to offset 31
    0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // amount(<type(1)><amount(8)><currency_code(20)><issuer(20)>) - Added on prepare to offset 36
    0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // fee - Added on prepare to offset 84
    0x73, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Signing Public Key (NULL offset 95)
    0x81, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_source(20) - Added on prepare to offset 130
    0x83, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_destination(20) - Added on prepare to offset 152
    0xF9, 0xEA, // Memo array and object start markers
    0x7C, 0x15,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, // MemoType (21 bytes) offset 176
    0x7D, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MemoData (32 bytes) offset 199
    0x7E, 0x03,
    0x00, 0x00, 0x00, // MemoFormat (3 bytes) offset 233
    0xE1, 0xF1,       // Memo array and object end markers
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit_details(138) - Added on prepare to offset 238
    // emit_details - NOTE : Considered additional 22 bytes for the callback scenario.
};

#define CANDIDATE_REBATE_PAYMENT_TX_SIZE \
    sizeof(CANDIDATE_REBATE_PAYMENT)
#define PREPARE_CANDIDATE_REBATE_PAYMENT_TX(evr_amount, to_address, memo_type, memo_data, memo_format) \
    {                                                                                                  \
        uint8_t *buf_out = CANDIDATE_REBATE_PAYMENT;                                                   \
        UINT32_TO_BUF((buf_out + 25), cur_ledger_seq + 1);                                             \
        UINT32_TO_BUF((buf_out + 31), cur_ledger_seq + 5);                                             \
        SET_AMOUNT_OUT((buf_out + 36), EVR_TOKEN, issuer_accid, evr_amount);                           \
        CANDIDATE_REBATE_COMMON((buf_out + 130), to_address, memo_type, memo_data, memo_format);       \
        etxn_details((buf_out + 238), CANDIDATE_REBATE_PAYMENT_TX_SIZE);                               \
        int64_t fee = etxn_fee_base(buf_out, CANDIDATE_REBATE_PAYMENT_TX_SIZE);                        \
        uint8_t *fee_ptr = buf_out + 84;                                                               \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee);                                                  \
    }

#define NOTIFY_OWNER(rebate_amount, owner, status, unique_id)                                                                                                            \
    {                                                                                                                                                                    \
        uint8_t emithash[HASH_SIZE];                                                                                                                                     \
        uint8_t *tx_ptr;                                                                                                                                                 \
        uint32_t tx_size;                                                                                                                                                \
        const uint8_t *memo_data_ptr = ((status == STATUS_VETOED) ? CANDIDATE_VETOED_RES : ((status == STATUS_ACCEPTED) ? CANDIDATE_ACCEPT_RES : CANDIDATE_EXPIRY_RES)); \
        if (rebate_amount > 0)                                                                                                                                           \
        {                                                                                                                                                                \
            PREPARE_CANDIDATE_REBATE_PAYMENT_TX(float_set(0, rebate_amount), owner, memo_data_ptr, unique_id, FORMAT_HEX);                                               \
            tx_ptr = CANDIDATE_REBATE_PAYMENT;                                                                                                                           \
            tx_size = CANDIDATE_REBATE_PAYMENT_TX_SIZE;                                                                                                                  \
        }                                                                                                                                                                \
        else                                                                                                                                                             \
        {                                                                                                                                                                \
            PREPARE_CANDIDATE_REBATE_MIN_PAYMENT_TX(float_set(0, rebate_amount), owner, memo_data_ptr, unique_id, FORMAT_HEX);                                           \
            tx_ptr = CANDIDATE_REBATE_MIN_PAYMENT;                                                                                                                       \
            tx_size = CANDIDATE_REBATE_MIN_PAYMENT_TX_SIZE;                                                                                                              \
        }                                                                                                                                                                \
        if (emit(SBUF(emithash), tx_ptr, tx_size) < 0)                                                                                                                   \
            rollback(SBUF("Evernode: EVR funding to candidate account failed."), 1);                                                                                     \
        trace(SBUF("emit hash: "), SBUF(emithash), 1);                                                                                                                   \
    }

// IOU Payment with single memo (send acquired funds from candidates).
uint8_t HEARTBEAT_FUND_PAYMENT[374] = {
    0x12, 0x00, 0x00,                   // transaction_type(ttPAYMENT)
    0x22, 0x80, 0x00, 0x00, 0x00,       // flags(tfCANONICAL)
    0x23, 0x00, 0x00, 0x00, 0x00,       // TAG_SOURCE
    0x24, 0x00, 0x00, 0x00, 0x00,       // sequence(0)
    0x2E, 0x00, 0x00, 0x00, 0x00,       // TAG DESTINATION
    0x20, 0x1A, 0x00, 0x00, 0x00, 0x00, // first_ledger_sequence(ledger_seq + 1) - Added on prepare to offset 25
    0x20, 0x1B, 0x00, 0x00, 0x00, 0x00, // last_ledger_sequence(ledger_seq + 5) - Added on prepare to offset 31
    0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // amount(<type(1)><amount(8)><currency_code(20)><issuer(20)>) - Added on prepare to offset 36
    0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // fee - Added on prepare to offset 84
    0x73, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Signing Public Key (NULL offset 95)
    0x81, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_source(20) - Added on prepare to offset 130
    0x83, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_destination(20) - Added on prepare to offset 152
    0xF9, 0xEA, // Memo array and object start markers
    0x7C, 0x13,
    0x65, 0x76, 0x6e, 0x52, 0x65, 0x77, 0x61, 0x72, 0x64, 0x50, 0x6f, 0x6f, 0x6c, 0x55, 0x70, 0x64, 0x61, 0x74, 0x65, // MemoType (19 bytes) offset 176
    0x7D,
    0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MemoData (32 bytes) offset 197
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x7E, 0x03,
    0x68, 0x65, 0x78, // MemoFormat (3 bytes) offset 231
    0xE1, 0xF1,       // Memo array and object end markers
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit_details(138) - Added on prepare to offset 236
    // emit_details - NOTE : Considered additional 22 bytes for the callback scenario.
};

#define HEARTBEAT_FUND_PAYMENT_TX_SIZE \
    sizeof(HEARTBEAT_FUND_PAYMENT)
#define PREPARE_HEARTBEAT_FUND_PAYMENT_TX(evr_amount, to_address, tx_ref)     \
    {                                                                         \
        uint8_t *buf_out = HEARTBEAT_FUND_PAYMENT;                            \
        UINT32_TO_BUF((buf_out + 25), cur_ledger_seq + 1);                    \
        UINT32_TO_BUF((buf_out + 31), cur_ledger_seq + 5);                    \
        SET_AMOUNT_OUT((buf_out + 36), EVR_TOKEN, issuer_accid, evr_amount);  \
        COPY_20BYTES((buf_out + 130), hook_accid);                            \
        COPY_20BYTES((buf_out + 152), to_address);                            \
        COPY_32BYTES((buf_out + 197), tx_ref);                                \
        etxn_details((buf_out + 236), HEARTBEAT_FUND_PAYMENT_TX_SIZE);        \
        int64_t fee = etxn_fee_base(buf_out, HEARTBEAT_FUND_PAYMENT_TX_SIZE); \
        uint8_t *fee_ptr = buf_out + 84;                                      \
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                \
    }

// Simple XRP Payment with single memo.
uint8_t HOOK_UPDATE_PAYMENT[328] = {
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
    0xF9, 0xEA, // Memo array and object start markers
    0x7C, 0x0D,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MemoType (13 bytes) offset 136
    0x7D, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MemoData (32 bytes) offset 151
    0x7E, 0x03,
    0x00, 0x00, 0x00, // MemoFormat (3 bytes) offset 185
    0xE1, 0xF1,       // Memo array and object end markers
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit_details(138) - Added on prepare to offset 190
    // emit_details - NOTE : Considered additional 22 bytes for the callback scenario.
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
        COPY_8BYTES((buf_out + 136), HOOK_UPDATE);                                                                       \
        COPY_4BYTES((buf_out + 136 + 8), HOOK_UPDATE + 8);                                                               \
        COPY_BYTE((buf_out + 136 + 12), HOOK_UPDATE + 12);                                                               \
        COPY_32BYTES((buf_out + 151), unique_id);                                                                        \
        COPY_2BYTES((buf_out + 185), FORMAT_HEX);                                                                        \
        COPY_BYTE((buf_out + 185 + 2), (FORMAT_HEX + 2));                                                                \
        etxn_details((buf_out + 190), HOOK_UPDATE_PAYMENT_TX_SIZE);                                                      \
        int64_t fee = etxn_fee_base(buf_out, HOOK_UPDATE_PAYMENT_TX_SIZE);                                               \
        buf_ptr = buf_out + 44;                                                                                          \
        _06_08_ENCODE_DROPS_FEE(buf_ptr, fee); /** Skip the fee check since this tx is sent to registry/governor hook.*/ \
    }

#define HANDLE_CANDIDATE_STATUS_CHANGE(candidate_type, status)                                                                                                       \
    {                                                                                                                                                                \
        /* For each candidate type we treat differently. */                                                                                                          \
        if (candidate_type == NEW_HOOK_CANDIDATE)                                                                                                                    \
        {                                                                                                                                                            \
            /* <epoch_count(uint8_t)><first_epoch_reward_quota(uint32_t)><epoch_reward_amount(uint32_t)><reward_start_moment(uint32_t)> */                           \
            uint8_t reward_configuration[REWARD_CONFIGURATION_VAL_SIZE];                                                                                             \
            /* <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)> */ \
            uint8_t reward_info[REWARD_INFO_VAL_SIZE];                                                                                                               \
            if (state_foreign(reward_configuration, REWARD_CONFIGURATION_VAL_SIZE, SBUF(CONF_REWARD_CONFIGURATION), FOREIGN_REF) < 0 ||                              \
                state_foreign(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO), FOREIGN_REF) < 0)                                                            \
                rollback(SBUF("Evernode: Could not get reward configuration or reward info states."), 1);                                                            \
                                                                                                                                                                     \
            const uint64_t last_election_completed_timestamp = UINT64_FROM_BUF(&governance_info[PROPOSAL_ELECTED_TIMESTAMP_OFFSET]);                                 \
            const uint8_t governance_mode = governance_info[GOVERNANCE_MODE_OFFSET];                                                                                 \
                                                                                                                                                                     \
            const uint8_t epoch_count = reward_configuration[EPOCH_COUNT_OFFSET];                                                                                    \
            const uint32_t first_epoch_reward_quota = UINT32_FROM_BUF(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET]);                                       \
            const uint32_t epoch_reward_amount = UINT32_FROM_BUF(&reward_configuration[EPOCH_REWARD_AMOUNT_OFFSET]);                                                 \
                                                                                                                                                                     \
            const uint64_t proposal_fee = UINT64_FROM_BUF(&candidate_id[CANDIDATE_PROPOSAL_FEE_OFFSET]);                                                             \
                                                                                                                                                                     \
            uint32_t reward_amount = (status == STATUS_ACCEPTED) ? 0 : (status == STATUS_EXPIRED) ? (proposal_fee / 2)                                               \
                                                                                                  : proposal_fee;                                                    \
            const uint64_t cur_moment_start_timestamp = GET_MOMENT_START_INDEX(cur_ledger_timestamp);                                                                \
                                                                                                                                                                     \
            if (status == STATUS_VETOED || status == STATUS_EXPIRED)                                                                                                 \
            {                                                                                                                                                        \
                uint8_t emithash[HASH_SIZE];                                                                                                                         \
                if (reward_amount > 0)                                                                                                                               \
                {                                                                                                                                                    \
                    ADD_TO_REWARD_POOL(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, reward_amount);                     \
                    /* Reading the heartbeat-hook account from states (Not yet set to states) */                                                                     \
                    uint8_t heartbeat_hook_accid[ACCOUNT_ID_SIZE];                                                                                                   \
                    if (state_foreign(SBUF(heartbeat_hook_accid), SBUF(CONF_HEARTBEAT_ADDR), FOREIGN_REF) < 0)                                                       \
                        rollback(SBUF("Evernode: Error getting heartbeat hook address from states."), 1);                                                            \
                                                                                                                                                                     \
                    /* Prepare EVR transaction to heartbeat hook account. */                                                                                         \
                    etxn_reserve(2);                                                                                                                                 \
                                                                                                                                                                     \
                    PREPARE_HEARTBEAT_FUND_PAYMENT_TX(float_set(0, reward_amount), heartbeat_hook_accid, txid);                                                      \
                    if (emit(SBUF(emithash), SBUF(HEARTBEAT_FUND_PAYMENT)) < 0)                                                                                      \
                        rollback(SBUF("Evernode: EVR funding to heartbeat hook account failed."), 1);                                                                \
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);                                                                                                   \
                }                                                                                                                                                    \
                else                                                                                                                                                 \
                {                                                                                                                                                    \
                    etxn_reserve(1);                                                                                                                                 \
                }                                                                                                                                                    \
                                                                                                                                                                     \
                /* Clear the proposal states. */                                                                                                                     \
                if (state_foreign_set(0, 0, SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0 || state_foreign_set(0, 0, SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) < 0)         \
                    rollback(SBUF("Evernode: Could not delete the candidate states."), 1);                                                                           \
            }                                                                                                                                                        \
            else if (status == STATUS_ACCEPTED)                                                                                                                      \
            {                                                                                                                                                        \
                /* Sending hook update trigger transactions to registry and heartbeat hooks. */                                                                      \
                etxn_reserve(3);                                                                                                                                     \
                                                                                                                                                                     \
                uint8_t emithash[HASH_SIZE];                                                                                                                         \
                                                                                                                                                                     \
                PREPARE_HOOK_UPDATE_PAYMENT_TX(1, heartbeat_accid, data_ptr);                                                                                        \
                if (emit(SBUF(emithash), SBUF(HOOK_UPDATE_PAYMENT)) < 0)                                                                                             \
                    rollback(SBUF("Evernode: Emitting heartbeat hook update trigger failed."), 1);                                                                   \
                                                                                                                                                                     \
                PREPARE_HOOK_UPDATE_PAYMENT_TX(1, registry_accid, data_ptr);                                                                                         \
                if (emit(SBUF(emithash), SBUF(HOOK_UPDATE_PAYMENT)) < 0)                                                                                             \
                    rollback(SBUF("Evernode: Emitting registry hook update trigger failed."), 1);                                                                    \
                                                                                                                                                                     \
                /* Update the governance info state. */                                                                                                              \
                COPY_32BYTES(&governance_info[ELECTED_PROPOSAL_UNIQUE_ID_OFFSET], data_ptr);                                                                         \
                UINT64_TO_BUF(&governance_info[PROPOSAL_ELECTED_TIMESTAMP_OFFSET], cur_moment_start_timestamp);                                                            \
                if (state_foreign_set(governance_info, GOVERNANCE_INFO_VAL_SIZE, SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0)                                        \
                    rollback(SBUF("Evernode: Could not set state for governance info."), 1);                                                                         \
            }                                                                                                                                                        \
                                                                                                                                                                     \
            if (status == STATUS_ACCEPTED || status == STATUS_VETOED || status == STATUS_EXPIRED)                                                                    \
                NOTIFY_OWNER((proposal_fee - reward_amount), candidate_id, status, data_ptr);                                                                        \
                                                                                                                                                                     \
            accept(SBUF("Evernode: New hook candidate status changed."), status);                                                                                    \
        }                                                                                                                                                            \
        else if (candidate_type == PILOTED_MODE_CANDIDATE)                                                                                                           \
        {                                                                                                                                                            \
            /* Change governance the mode. */                                                                                                                        \
            governance_info[GOVERNANCE_MODE_OFFSET] = PILOTED;                                                                                                       \
            if (state_foreign_set(governance_info, GOVERNANCE_INFO_VAL_SIZE, SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0)                                            \
                rollback(SBUF("Evernode: Could not set state for governance_game info."), 1);                                                                        \
                                                                                                                                                                     \
            /* Clear piloted mode candidate if piloted mode is elected. */                                                                                           \
            if (state_foreign_set(0, 0, SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)                                                                                    \
                rollback(SBUF("Evernode: Could not set state for piloted mode candidate."), 1);                                                                      \
                                                                                                                                                                     \
            accept(SBUF("Evernode: Piloted mode candidate status changed."), status);                                                                                \
        }                                                                                                                                                            \
        else if (candidate_type == DUD_HOST_CANDIDATE)                                                                                                               \
        {                                                                                                                                                            \
            HOST_ADDR_KEY(data_ptr + DUD_HOST_CANDID_ADDRESS_OFFSET);                                                                                                \
            /* Check for registration entry. */                                                                                                                      \
            if (state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) == DOESNT_EXIST)                                                                    \
                rollback(SBUF("Evernode: This dud host is not registered."), 1);                                                                                     \
                                                                                                                                                                     \
            TOKEN_ID_KEY((uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET));                                                                                             \
            /* Check for token id entry.*/                                                                                                                           \
            if (state_foreign(SBUF(token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) == DOESNT_EXIST)                                                                      \
                rollback(SBUF("Evernode: This dud host is not registered."), 1);                                                                                     \
                                                                                                                                                                     \
            /* <epoch_count(uint8_t)><first_epoch_reward_quota(uint32_t)><epoch_reward_amount(uint32_t)><reward_start_moment(uint32_t)> */                           \
            uint8_t reward_configuration[REWARD_CONFIGURATION_VAL_SIZE];                                                                                             \
            /* <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)> */ \
            uint8_t reward_info[REWARD_INFO_VAL_SIZE];                                                                                                               \
            uint8_t epoch_count = 0;                                                                                                                                 \
            uint32_t first_epoch_reward_quota, epoch_reward_amount = 0;                                                                                              \
                                                                                                                                                                     \
            if (state_foreign(reward_configuration, REWARD_CONFIGURATION_VAL_SIZE, SBUF(CONF_REWARD_CONFIGURATION), FOREIGN_REF) < 0 ||                              \
                state_foreign(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO), FOREIGN_REF) < 0)                                                            \
                rollback(SBUF("Evernode: Could not get reward configuration or reward info states."), 1);                                                            \
                                                                                                                                                                     \
            epoch_count = reward_configuration[EPOCH_COUNT_OFFSET];                                                                                                  \
            first_epoch_reward_quota = UINT32_FROM_BUF(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET]);                                                      \
            epoch_reward_amount = UINT32_FROM_BUF(&reward_configuration[EPOCH_REWARD_AMOUNT_OFFSET]);                                                                \
                                                                                                                                                                     \
            HANDLE_HOST_PRUNE(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, 1);                                          \
                                                                                                                                                                     \
            /* Clear the dud host candidate if the host is removed. */                                                                                               \
            if (state_foreign_set(0, 0, SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)                                                                                    \
                rollback(SBUF("Evernode: Could not set state for piloted mode candidate."), 1);                                                                      \
                                                                                                                                                                     \
            accept(SBUF("Evernode: Dud host candidate status changed."), status);                                                                                    \
        }                                                                                                                                                            \
    }
#endif
