#ifndef EVERNODE_INCLUDED
#define EVERNODE_INCLUDED 1

#include "constants.h"
#include "transactions.h"

/****** Field and Type codes ******/
#define ARRAY 0xF0U
#define OBJECT 0xE0U
#define MEMOS 0x9
#define MEMO 0xA
#define HOOKS 0xB
#define HOOK 0xE
#define HOOK_GRANTS 0x14
#define HOOK_GRANT 0x18
#define HOOK_PARAMS 0x13
#define HOOK_PARAM 0x17
#define HOOK_PARAM_NAME 0x18
#define HOOK_PARAM_VALUE 0x19
#define END 1
#define MEMO_TYPE 0xC
#define MEMO_DATA 0xD
#define MEMO_FORMAT 0xE
#define AUTHORIZE 0x05

/**************************************************************************/
/****************************Utility macros********************************/
/**************************************************************************/

#define GET_TOKEN_CURRENCY(token)                                                       \
    {                                                                                   \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, token[0], token[1], token[2], 0, 0, 0, 0, 0 \
    }

const uint8_t evr_currency[20] = GET_TOKEN_CURRENCY(EVR_TOKEN);

// Checks for EVR currency issued by issuer account.
#define IS_EVR(amount_buffer, issuer_accid)                \
    (BUFFER_EQUAL_20((amount_buffer + 8), evr_currency) && \
     BUFFER_EQUAL_20((amount_buffer + 28), issuer_accid))

#define MAX(num1, num2) \
    ((num1 > num2) ? num1 : num2)

#define MIN(num1, num2) \
    ((num1 > num2) ? num2 : num1)

#define CEIL(dividend, divisor) \
    ((dividend / divisor) + ((dividend % divisor) != 0))

// It gives -29 (EXPONENT_UNDERSIZED) when balance is zero. Need to read and experiment more.
#define IS_FLOAT_ZERO(float) \
    (float == 0 || float == -29)

#define BUFFER_EQUAL_1(buf1, buf2) \
    (*(uint8_t *)(buf1) == *(uint8_t *)(buf2))

#define BUFFER_EQUAL_2(buf1, buf2) \
    (*(uint16_t *)(buf1) == *(uint16_t *)(buf2))

#define BUFFER_EQUAL_4(buf1, buf2) \
    (*(uint32_t *)(buf1) == *(uint32_t *)(buf2))

#define BUFFER_EQUAL_8(buf1, buf2) \
    (*(uint64_t *)(buf1) == *(uint64_t *)(buf2))

#define BUFFER_EQUAL_20(buf1, buf2)            \
    (BUFFER_EQUAL_8(buf1, buf2) &&             \
     BUFFER_EQUAL_8((buf1 + 8), (buf2 + 8)) && \
     BUFFER_EQUAL_4((buf1 + 16), (buf2 + 16)))

#define BUFFER_EQUAL_28(buf1, buf2)              \
    (BUFFER_EQUAL_8(buf1, buf2) &&               \
     BUFFER_EQUAL_8((buf1 + 8), (buf2 + 8)) &&   \
     BUFFER_EQUAL_8((buf1 + 16), (buf2 + 16)) && \
     BUFFER_EQUAL_4((buf1 + 24), (buf2 + 24)))

#define BUFFER_EQUAL_32(buf1, buf2)              \
    (BUFFER_EQUAL_8(buf1, buf2) &&               \
     BUFFER_EQUAL_8((buf1 + 8), (buf2 + 8)) &&   \
     BUFFER_EQUAL_8((buf1 + 16), (buf2 + 16)) && \
     BUFFER_EQUAL_8((buf1 + 24), (buf2 + 24)))

#define COPY_BYTE(lhsbuf, rhsbuf) \
    *(uint8_t *)(lhsbuf) = *(uint8_t *)(rhsbuf);

#define COPY_2BYTES(lhsbuf, rhsbuf) \
    *(uint16_t *)(lhsbuf) = *(uint16_t *)(rhsbuf);

#define COPY_4BYTES(lhsbuf, rhsbuf) \
    *(uint32_t *)(lhsbuf) = *(uint32_t *)(rhsbuf);

#define COPY_8BYTES(lhsbuf, rhsbuf) \
    *(uint64_t *)(lhsbuf) = *(uint64_t *)(rhsbuf);

#define COPY_10BYTES(lhsbuf, rhsbuf) \
    COPY_8BYTES(lhsbuf, rhsbuf);     \
    COPY_2BYTES((lhsbuf + 8), (rhsbuf + 8));

#define COPY_16BYTES(lhsbuf, rhsbuf) \
    COPY_8BYTES(lhsbuf, rhsbuf);     \
    COPY_8BYTES((lhsbuf + 8), (rhsbuf + 8));

#define COPY_20BYTES(lhsbuf, rhsbuf)         \
    COPY_8BYTES(lhsbuf, rhsbuf);             \
    COPY_8BYTES((lhsbuf + 8), (rhsbuf + 8)); \
    COPY_4BYTES((lhsbuf + 16), (rhsbuf + 16));

#define COPY_32BYTES(lhsbuf, rhsbuf)           \
    COPY_8BYTES(lhsbuf, rhsbuf);               \
    COPY_8BYTES((lhsbuf + 8), (rhsbuf + 8));   \
    COPY_8BYTES((lhsbuf + 16), (rhsbuf + 16)); \
    COPY_8BYTES((lhsbuf + 24), (rhsbuf + 24));

#define COPY_40BYTES(lhsbuf, rhsbuf) \
    COPY_32BYTES(lhsbuf, rhsbuf);    \
    COPY_8BYTES((lhsbuf + 32), (rhsbuf + 32));

#define CLEAR_BYTE(buf) \
    *(uint8_t *)(buf) = 0;

#define CLEAR_2BYTES(buf) \
    UINT16_TO_BUF(buf, 0);

#define CLEAR_4BYTES(buf) \
    UINT32_TO_BUF(buf, 0);

#define CLEAR_8BYTES(buf) \
    UINT64_TO_BUF(buf, 0);

#define CLEAR_20BYTES(buf)   \
    CLEAR_8BYTES(buf);       \
    CLEAR_8BYTES((buf + 8)); \
    CLEAR_4BYTES((buf + 16));

#define CLEAR_32BYTES(buf)    \
    CLEAR_8BYTES(buf);        \
    CLEAR_8BYTES((buf + 8));  \
    CLEAR_8BYTES((buf + 16)); \
    CLEAR_8BYTES((buf + 24));

#define IS_BUFFER_EMPTY_1(buf) \
    (*(uint8_t *)(buf) == 0)

#define IS_BUFFER_EMPTY_2(buf) \
    (*(uint16_t *)(buf) == 0)

#define IS_BUFFER_EMPTY_4(buf) \
    (*(uint32_t *)(buf) == 0)

#define IS_BUFFER_EMPTY_8(buf) \
    (*(uint64_t *)(buf) == 0)

#define IS_BUFFER_EMPTY_20(buf)      \
    (IS_BUFFER_EMPTY_8(buf) &&       \
     IS_BUFFER_EMPTY_8((buf + 8)) && \
     IS_BUFFER_EMPTY_4((buf + 16)))

#define IS_BUFFER_EMPTY_32(buf)       \
    (IS_BUFFER_EMPTY_8(buf) &&        \
     IS_BUFFER_EMPTY_8((buf + 8)) &&  \
     IS_BUFFER_EMPTY_8((buf + 16)) && \
     IS_BUFFER_EMPTY_8((buf + 24)))

// Domain related comparer macros.

#define EQUAL_FORMAT_HEX(buf, len)      \
    (sizeof(FORMAT_HEX) == (len + 1) && \
     BUFFER_EQUAL_2(buf, FORMAT_HEX) && \
     BUFFER_EQUAL_1((buf + 2), (FORMAT_HEX + 2)))

#define EQUAL_FORMAT_TEXT(buf, len)      \
    (sizeof(FORMAT_TEXT) == (len + 1) && \
     BUFFER_EQUAL_8(buf, FORMAT_TEXT) && \
     BUFFER_EQUAL_2((buf + 8), (FORMAT_TEXT + 8)))

#define EQUAL_FORMAT_BASE64(buf, len)      \
    (sizeof(FORMAT_BASE64) == (len + 1) && \
     BUFFER_EQUAL_4(buf, FORMAT_BASE64) && \
     BUFFER_EQUAL_2((buf + 4), (FORMAT_BASE64 + 4)))

#define EQUAL_FORMAT_JSON(buf, len)      \
    (sizeof(FORMAT_JSON) == (len + 1) && \
     BUFFER_EQUAL_8(buf, FORMAT_JSON) && \
     BUFFER_EQUAL_1((buf + 8), (FORMAT_JSON + 8)))

#define EQUAL_EVR_HOST_PREFIX(buf)                \
    (BUFFER_EQUAL_4(buf, EVR_HOST) &&             \
     BUFFER_EQUAL_2((buf + 4), (EVR_HOST + 4)) && \
     BUFFER_EQUAL_1((buf + 6), (EVR_HOST + 6)))

#define EQUAL_HOOK_UPDATE(buf, len)                  \
    (sizeof(HOOK_UPDATE) == (len + 1) &&             \
     BUFFER_EQUAL_8(buf, HOOK_UPDATE) &&             \
     BUFFER_EQUAL_4((buf + 8), (HOOK_UPDATE + 8)) && \
     BUFFER_EQUAL_1((buf + 12), (HOOK_UPDATE + 12)))

#define EQUAL_CANDIDATE_VOTE(buf, len)      \
    (sizeof(CANDIDATE_VOTE) == (len + 1) && \
     BUFFER_EQUAL_8(buf, CANDIDATE_VOTE) && \
     BUFFER_EQUAL_8(buf + 8, CANDIDATE_VOTE + 8))

#define EQUAL_NEW_HOOK_CAND_PREFIX(buf)                   \
    (BUFFER_EQUAL_8(buf, NEW_HOOK_CAND_PREFIX) &&         \
     BUFFER_EQUAL_2(buf + 8, NEW_HOOK_CAND_PREFIX + 8) && \
     BUFFER_EQUAL_1(buf + 10, NEW_HOOK_CAND_PREFIX + 10))

#define EQUAL_PILOTED_MODE_CAND_PREFIX(buf)                     \
    (BUFFER_EQUAL_8(buf, PILOTED_MODE_CAND_PREFIX) &&           \
     BUFFER_EQUAL_4(buf + 8, PILOTED_MODE_CAND_PREFIX + 8) &&   \
     BUFFER_EQUAL_2(buf + 12, PILOTED_MODE_CAND_PREFIX + 12) && \
     BUFFER_EQUAL_1(buf + 14, PILOTED_MODE_CAND_PREFIX + 14))

#define EQUAL_DUD_HOST_CAND_PREFIX(buf)                   \
    (BUFFER_EQUAL_8(buf, DUD_HOST_CAND_PREFIX) &&         \
     BUFFER_EQUAL_2(buf + 8, DUD_HOST_CAND_PREFIX + 8) && \
     BUFFER_EQUAL_1(buf + 10, DUD_HOST_CAND_PREFIX + 10))

// Provide m >= 1 to indicate in which code line macro will hit.
// Provide n >= 1 to indicate how many times the macro will be hit on the line of code.
// e.g. if it is in a loop that loops 10 times n = 10
// If it is used 3 times inside a macro use m = 1,2,3
// We need to dump the iou amount into a buffer.
// by supplying -1 as the fieldcode we tell float_sto not to prefix an actual STO header on the field.
#define SET_AMOUNT_OUT_GUARDM(amt_out, token, issuer, amount, n, m)                 \
    {                                                                               \
        uint8_t currency[20] = GET_TOKEN_CURRENCY(token);                           \
        if (float_sto(amt_out, 49, currency, 20, issuer, 20, amount, sfAmount) < 0) \
            rollback(SBUF("Evernode: Could not dump token amount into sto"), 1);    \
        COPY_20BYTES((amt_out + 9), currency);                                      \
        COPY_20BYTES((amt_out + 29), issuer);                                       \
        if (amount == 0)                                                            \
            amt_out[1] = amt_out[1] & 0b10111111; /* Set the sign bit to 0.*/       \
    }

#define SET_AMOUNT_OUT_GUARD(amt_out, token, issuer, amount, n) \
    SET_AMOUNT_OUT_GUARDM(amt_out, token, issuer, amount, n, 1)

#define SET_AMOUNT_OUT(amt_out, token, issuer, amount) \
    SET_AMOUNT_OUT_GUARDM(amt_out, token, issuer, amount, 1, 1)

#define GET_MEMO(memo_lookup, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len) \
    {                                                                                                                               \
        /* since our memos are in a buffer inside the hook (as opposed to being a slot) we use the sto api with it                  \
        the sto apis probe into a serialized object returning offsets and lengths of subfields or array entries */                  \
        memo_ptr = SUB_OFFSET(memo_lookup) + memos;                                                                                 \
        memo_len = SUB_LENGTH(memo_lookup);                                                                                         \
        /* memos are nested inside an actual memo object, so we need to subfield                                                    \
        / equivalently in JSON this would look like memo_array[i]["Memo"] */                                                        \
        memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemo);                                                                     \
        memo_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;                                                                              \
        memo_len = SUB_LENGTH(memo_lookup);                                                                                         \
        if (memo_lookup < 0)                                                                                                        \
            accept(SBUF("Evernode: Incoming txn had a blank sfMemos."), 1);                                                         \
        memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemoType);                                                                 \
        type_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;                                                                              \
        type_len = SUB_LENGTH(memo_lookup);                                                                                         \
        /* trace(SBUF("type in hex: "), type_ptr, type_len, 1); */                                                                  \
        memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemoFormat);                                                               \
        format_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;                                                                            \
        format_len = SUB_LENGTH(memo_lookup);                                                                                       \
        /* trace(SBUF("format in hex: "), format_ptr, format_len, 1); */                                                            \
        memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemoData);                                                                 \
        data_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;                                                                              \
        data_len = SUB_LENGTH(memo_lookup);                                                                                         \
        /* trace(SBUF("data in hex: "), data_ptr, data_len, 1); // Text data is in hex format. */                                   \
    }

#define GET_CONF_VALUE(value, key, error_buf)                                       \
    {                                                                               \
        uint8_t size = sizeof(value);                                               \
        uint8_t value_buf[size];                                                    \
        int64_t state_res = state_foreign(SBUF(value_buf), SBUF(key), FOREIGN_REF); \
        if (state_res < 0)                                                          \
            rollback(SBUF(error_buf), 1);                                           \
        switch (size)                                                               \
        {                                                                           \
        case 2:                                                                     \
            value = UINT16_FROM_BUF(value_buf);                                     \
            break;                                                                  \
        case 4:                                                                     \
            value = UINT32_FROM_BUF(value_buf);                                     \
            break;                                                                  \
        case 8:                                                                     \
            value = UINT64_FROM_BUF(value_buf);                                     \
            break;                                                                  \
        default:                                                                    \
            rollback(SBUF("Evernode: Invalid state value."), 1);                    \
            break;                                                                  \
        }                                                                           \
    }

#define GET_FLOAT_CONF_VALUE(value, def_mentissa, def_exponent, key, error_buf)     \
    {                                                                               \
        uint8_t value_buf[8];                                                       \
        int64_t state_res = state_foreign(SBUF(value_buf), SBUF(key), FOREIGN_REF); \
        if (state_res == DOESNT_EXIST)                                              \
        {                                                                           \
            value = float_set(def_exponent, def_mentissa);                          \
            INT64_TO_BUF(value_buf, value);                                         \
        }                                                                           \
        else                                                                        \
            value = INT64_FROM_BUF(value_buf);                                      \
                                                                                    \
        if (state_res == DOESNT_EXIST)                                              \
        {                                                                           \
            if (state_foreign_set(SBUF(value_buf), SBUF(key), FOREIGN_REF) < 0)     \
                rollback(SBUF(error_buf), 1);                                       \
        }                                                                           \
    }

#define GET_HOST_COUNT(host_count)                                                                  \
    {                                                                                               \
        uint8_t host_count_buf[4] = {0};                                                            \
        host_count = 0;                                                                             \
        if (state_foreign(SBUF(host_count_buf), SBUF(STK_HOST_COUNT), FOREIGN_REF) != DOESNT_EXIST) \
            host_count = UINT32_FROM_BUF(host_count_buf);                                           \
    }

#define GET_MOMENT(idx) \
    (prev_transition_moment + ((idx - moment_base_idx) / moment_size))

#define GET_NEXT_MOMENT_START_INDEX(idx) \
    (moment_base_idx + ((((idx - moment_base_idx) / moment_size) + 1) * moment_size))

#define GET_MOMENT_START_INDEX(idx) \
    (moment_base_idx + ((((idx - moment_base_idx) / moment_size)) * moment_size))

#define SET_HOST_COUNT(host_count)                                                          \
    {                                                                                       \
        uint8_t host_count_buf[4] = {0};                                                    \
        UINT32_TO_BUF(host_count_buf, host_count);                                          \
        if (state_foreign_set(SBUF(host_count_buf), SBUF(STK_HOST_COUNT), FOREIGN_REF) < 0) \
            rollback(SBUF("Evernode: Could not set default state for host count."), 1);     \
    }

#define IS_REG_TOKEN_EXIST(account, token_id, token_exists)                                                 \
    {                                                                                                       \
        token_exists = 0;                                                                                   \
        uint8_t keylet[34];                                                                                 \
        UINT16_TO_BUF(keylet, ltURI_TOKEN);                                                                 \
        COPY_32BYTES((keylet + 2), token_id);                                                               \
        int64_t token_slot = slot_set(keylet, 34, 0);                                                       \
        if (token_slot < 0)                                                                                 \
            rollback(SBUF("Evernode: Could not set ledger uri token keylet in slot"), 10);                  \
        token_slot = slot_subfield(token_slot, sfOwner, 0);                                                 \
        if (token_slot < 0)                                                                                 \
            rollback(SBUF("Evernode: Could not find sfOwner for uri token"), 1);                            \
        uint8_t owner[ACCOUNT_ID_SIZE] = {0};                                                               \
        token_exists = slot(SBUF(owner), token_slot) == ACCOUNT_ID_SIZE && BUFFER_EQUAL_20(owner, account); \
    }

#define POW_OF_TWO(exp, output)              \
    {                                        \
        uint16_t val = 32768;                \
        val = (uint16_t)(val >> (15 - exp)); \
        output = val;                        \
    }

#define GET_EPOCH_REWARD_QUOTA(epoch, first_epoch_reward_quota, quota) \
    {                                                                  \
        uint32_t div = 1;                                              \
        if (epoch > 1)                                                 \
            POW_OF_TWO((epoch - 1), div);                              \
        quota = first_epoch_reward_quota / div;                        \
    }

#define PREPARE_EPOCH_REWARD_INFO(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, is_heartbeat, reward_pool_amount_ref, reward_amount_ref) \
    {                                                                                                                                                                                \
        const uint8_t epoch = reward_info[EPOCH_OFFSET];                                                                                                                             \
        uint32_t reward_quota;                                                                                                                                                       \
        GET_EPOCH_REWARD_QUOTA(epoch, first_epoch_reward_quota, reward_quota);                                                                                                       \
        uint32_t prev_moment_active_host_count = UINT32_FROM_BUF(&reward_info[PREV_MOMENT_ACTIVE_HOST_COUNT_OFFSET]);                                                                \
        const uint32_t cur_moment_active_host_count = UINT32_FROM_BUF(&reward_info[CUR_MOMENT_ACTIVE_HOST_COUNT_OFFSET]);                                                            \
        const uint8_t *pool_ptr = &reward_info[EPOCH_POOL_OFFSET];                                                                                                                   \
        reward_pool_amount_ref = INT64_FROM_BUF(pool_ptr);                                                                                                                           \
        const uint32_t saved_moment = UINT32_FROM_BUF(&reward_info[SAVED_MOMENT_OFFSET]);                                                                                            \
        const uint32_t cur_moment = GET_MOMENT(cur_idx);                                                                                                                             \
        /* If this is a new moment, update the host counts. */                                                                                                                       \
        if (saved_moment != cur_moment)                                                                                                                                              \
        {                                                                                                                                                                            \
            /* Remove previous moment data and move current moment data to previous moment. */                                                                                       \
            UINT32_TO_BUF(&reward_info[SAVED_MOMENT_OFFSET], cur_moment);                                                                                                            \
            /* If the saved moment is not cur_moment - 1, We've missed some moments. Means there was no heartbeat received in previous moment. */                                    \
            prev_moment_active_host_count = ((saved_moment == cur_moment - 1) ? cur_moment_active_host_count : 0);                                                                   \
            UINT32_TO_BUF(&reward_info[PREV_MOMENT_ACTIVE_HOST_COUNT_OFFSET], prev_moment_active_host_count);                                                                        \
            /* If the macro is called from heartbeat intialte cur moment active host count as 1. */                                                                                  \
            UINT32_TO_BUF(&reward_info[CUR_MOMENT_ACTIVE_HOST_COUNT_OFFSET], (is_heartbeat ? 1 : 0));                                                                                \
        }                                                                                                                                                                            \
        /* If the macro is called from heartbeat increase cur moment active host count by 1. */                                                                                      \
        else if (is_heartbeat)                                                                                                                                                       \
        {                                                                                                                                                                            \
            UINT32_TO_BUF(&reward_info[CUR_MOMENT_ACTIVE_HOST_COUNT_OFFSET], (cur_moment_active_host_count + 1));                                                                    \
        }                                                                                                                                                                            \
        /* Reward pool amount is less than the reward quota for the moment, Increment the epoch. And add the remaining to the next epoch pool. */                                    \
        if (float_compare(reward_pool_amount_ref, float_set(0, reward_quota), COMPARE_LESS) == 1)                                                                                    \
        {                                                                                                                                                                            \
            /* If the current epoch is < epoch count increment otherwise skip. */                                                                                                    \
            if (epoch < epoch_count)                                                                                                                                                 \
            {                                                                                                                                                                        \
                reward_pool_amount_ref = float_sum(float_set(0, epoch_reward_amount), reward_pool_amount_ref);                                                                       \
                INT64_TO_BUF(pool_ptr, reward_pool_amount_ref);                                                                                                                      \
                reward_info[EPOCH_OFFSET] = epoch + 1;                                                                                                                               \
                /* When epoch incremented by 1, reward quota halves. */                                                                                                              \
                reward_quota = reward_quota / 2;                                                                                                                                     \
            }                                                                                                                                                                        \
            else                                                                                                                                                                     \
            {                                                                                                                                                                        \
                reward_quota = 0;                                                                                                                                                    \
            }                                                                                                                                                                        \
        }                                                                                                                                                                            \
        /* Calculate the reward quota for the current epoch. */                                                                                                                      \
        /* Use float division only if modulo is not zero to reduce floating point complications. */                                                                                  \
        if (prev_moment_active_host_count == 0)                                                                                                                                      \
            reward_amount_ref = float_set(0, 0);                                                                                                                                     \
        else if (reward_quota % prev_moment_active_host_count == 0)                                                                                                                  \
            reward_amount_ref = float_set(0, (reward_quota / prev_moment_active_host_count));                                                                                        \
        else                                                                                                                                                                         \
            reward_amount_ref = float_divide(float_set(0, reward_quota), float_set(0, prev_moment_active_host_count));                                                               \
    }

#define CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee)                                                                 \
    {                                                                                                                \
        if (fee > 0)                                                                                                 \
        {                                                                                                            \
            uint64_t max_trx_fee;                                                                                    \
            GET_CONF_VALUE(max_trx_fee, CONF_MAX_EMIT_TRX_FEE, "Evernode: Could not get the maximum trx emit fee."); \
            if (fee >= max_trx_fee)                                                                                  \
                rollback(SBUF("Evernode: Too large transaction fee."), 1);                                           \
        }                                                                                                            \
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                                                       \
    }

#define IS_HOST_PRUNABLE(host_addr, is_prunable)                                                                                                       \
    {                                                                                                                                                  \
        is_prunable = 0;                                                                                                                               \
        int64_t registration_timestamp = UINT64_FROM_BUF(&host_addr[HOST_REG_TIMESTAMP_OFFSET]);                                                       \
                                                                                                                                                       \
        uint8_t *last_active_idx_ptr = &host_addr[HOST_HEARTBEAT_TIMESTAMP_OFFSET];                                                                    \
        int64_t last_active_idx = INT64_FROM_BUF(last_active_idx_ptr);                                                                                 \
        /* If host haven't sent a heartbeat yet, take the registration ledger as the last active ledger. */                                            \
        if (last_active_idx == 0)                                                                                                                      \
            last_active_idx = registration_timestamp;                                                                                                  \
        const int64_t heartbeat_delay = (cur_idx - last_active_idx) / moment_size;                                                                     \
                                                                                                                                                       \
        /* Take the maximun tolerable downtime from config. */                                                                                         \
        uint16_t max_tolerable_downtime;                                                                                                               \
        GET_CONF_VALUE(max_tolerable_downtime, CONF_MAX_TOLERABLE_DOWNTIME, "Evernode: Could not get the maximum tolerable downtime from the state."); \
                                                                                                                                                       \
        is_prunable = heartbeat_delay < max_tolerable_downtime ? 0 : 1;                                                                                \
    }

#define ADD_TO_REWARD_POOL(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, amount_float)                                 \
    {                                                                                                                                                              \
        const uint8_t *pool_ptr = &reward_info[EPOCH_POOL_OFFSET];                                                                                                 \
        INT64_TO_BUF(pool_ptr, float_sum(INT64_FROM_BUF(pool_ptr), amount_float)); /* Prepare reward info to update host counts and epoch. */                      \
        int64_t reward_pool_amount, reward_amount;                                                                                                                 \
        PREPARE_EPOCH_REWARD_INFO(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, 0, reward_pool_amount, reward_amount); \
                                                                                                                                                                   \
        if (state_foreign_set(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO), FOREIGN_REF) < 0)                                                          \
            rollback(SBUF("Evernode: Could not set state for reward info."), 1);                                                                                   \
    }

#define VALIDATE_GOVERNANCE_ELIGIBILITY(host_addr, cur_ledger_timestamp, min_eligibility_period, eligible_for_governance, do_rollback) \
    {                                                                                                                                  \
        eligible_for_governance = 1;                                                                                                   \
        uint64_t registration_timestamp = UINT64_FROM_BUF(&host_addr[HOST_REG_TIMESTAMP_OFFSET]);                                      \
                                                                                                                                       \
        if ((cur_ledger_timestamp - registration_timestamp) < min_eligibility_period)                                                  \
        {                                                                                                                              \
            eligible_for_governance = 0;                                                                                               \
            if (do_rollback == 1)                                                                                                      \
                rollback(SBUF("Evernode: Host is not eligible for proposing due to immaturity."), 1);                                  \
        }                                                                                                                              \
                                                                                                                                       \
        int is_prunable = 0;                                                                                                           \
        IS_HOST_PRUNABLE(host_addr, is_prunable);                                                                                      \
        if (is_prunable)                                                                                                               \
        {                                                                                                                              \
            eligible_for_governance = 0;                                                                                               \
            if (do_rollback == 1)                                                                                                      \
                rollback(SBUF("Evernode: Host is not eligible for proposing due to inactiveness."), 1);                                \
        }                                                                                                                              \
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

#define HANDLE_HOOK_UPDATE(hash_offset)                                                                                                                       \
    {                                                                                                                                                         \
        /* We accept only the hook update transaction from governor account. */                                                                               \
        if (!BUFFER_EQUAL_20(state_hook_accid, account_field))                                                                                                \
            rollback(SBUF("Evernode: Only governor is allowed to send hook update trigger."), 1);                                                             \
                                                                                                                                                              \
        /* <governance_mode(1)><last_candidate_idx(4)><voter_base_count(4)><voter_base_count_changed_timestamp(8)><foundation_last_voted_candidate_idx(4)> */ \
        /* <elected_proposal_unique_id(32)><proposal_elected_timestamp(8)><updated_hook_count(1)><foundation_vote_flag(1)> */                                 \
        uint8_t governance_game_info[GOVERNANCE_INFO_VAL_SIZE];                                                                                               \
        if (state_foreign(SBUF(governance_game_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0)                                                            \
            rollback(SBUF("Evernode: Could not get state governance_game info."), 1);                                                                         \
                                                                                                                                                              \
        if (!BUFFER_EQUAL_32(data_ptr, &governance_game_info[ELECTED_PROPOSAL_UNIQUE_ID_OFFSET]))                                                             \
            rollback(SBUF("Evernode: Candidate unique id is invalid."), 1);                                                                                   \
                                                                                                                                                              \
        CANDIDATE_ID_KEY(data_ptr);                                                                                                                           \
        /* <owner_address(20)><candidate_idx(4)><short_name(20)><created_timestamp(8)><proposal_fee(8)><positive_vote_count(4)> */                            \
        /* <last_vote_timestamp(8)><status(1)><status_change_timestamp(8)><foundation_vote_status(1)> */                                                      \
        uint8_t candidate_id[CANDIDATE_ID_VAL_SIZE];                                                                                                          \
        if (state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)                                                                       \
            rollback(SBUF("Evernode: Error getting a candidate for the given id."), 1);                                                                       \
                                                                                                                                                              \
        /* As first 20 bytes of "candidate_id" represents owner address.*/                                                                                    \
        CANDIDATE_OWNER_KEY(candidate_id);                                                                                                                    \
        /* <GOVERNOR_HASH(32)><REGISTRY_HASH(32)><HEARTBEAT_HASH(32)> */                                                                                      \
        uint8_t candidate_owner[CANDIDATE_OWNER_VAL_SIZE];                                                                                                    \
        if (state_foreign(SBUF(candidate_owner), SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) < 0)                                                                 \
            rollback(SBUF("Evernode: Could not get candidate owner state."), 1);                                                                              \
                                                                                                                                                              \
        etxn_reserve(1);                                                                                                                                      \
        if (reserved == STRONG_HOOK)                                                                                                                          \
        {                                                                                                                                                     \
            uint8_t hash_arr[HASH_SIZE * 4];                                                                                                                  \
            COPY_32BYTES(hash_arr, &candidate_owner[hash_offset]);                                                                                            \
            CLEAR_32BYTES(&hash_arr[HASH_SIZE]);                                                                                                              \
            CLEAR_32BYTES(&hash_arr[HASH_SIZE * 2]);                                                                                                          \
            CLEAR_32BYTES(&hash_arr[HASH_SIZE * 3]);                                                                                                          \
                                                                                                                                                              \
            int tx_size;                                                                                                                                      \
            PREPARE_SET_HOOK_TRANSACTION_TX(hash_arr, NAMESPACE, data_ptr, PARAM_STATE_HOOK_KEY, HASH_SIZE, state_hook_accid, ACCOUNT_ID_SIZE, tx_size);      \
            uint8_t emithash[HASH_SIZE];                                                                                                                      \
            if (emit(SBUF(emithash), SET_HOOK_TRANSACTION, tx_size) < 0)                                                                                      \
                rollback(SBUF("Evernode: Emitting set hook failed"), 1);                                                                                      \
            trace(SBUF("emit hash: "), SBUF(emithash), 1);                                                                                                    \
                                                                                                                                                              \
            if (hook_again() != 1)                                                                                                                            \
                rollback(SBUF("Evernode: Hook again failed on update hook."), 1);                                                                             \
            accept(SBUF("Evernode: Successfully applied the hook update."), 0);                                                                               \
        }                                                                                                                                                     \
        else if (reserved == AGAIN_HOOK)                                                                                                                      \
        {                                                                                                                                                     \
            PREPARE_HOOK_UPDATE_RES_PAYMENT_TX(1, state_hook_accid, data_ptr);                                                                                \
            uint8_t emithash[HASH_SIZE];                                                                                                                      \
            if (emit(SBUF(emithash), SBUF(HOOK_UPDATE_RES_PAYMENT)) < 0)                                                                                      \
                rollback(SBUF("Evernode: Emitting txn failed"), 1);                                                                                           \
            trace(SBUF("emit hash: "), SBUF(emithash), 1);                                                                                                    \
            accept(SBUF("Evernode: Hook update results sent successfully."), 0);                                                                              \
        }                                                                                                                                                     \
    }

#define CANDIDATE_TYPE(id) \
    (*(uint8_t *)(id + 4))

#define PREPARE_VOTE_INFO(cur_moment, last_vote_moment, last_vote_timestamp, voter_base_count, governance_info, candidate_id, supported_count, status)              \
    {                                                                                                                                                               \
        const uint64_t last_election_completed_timestamp = UINT64_FROM_BUF(&governance_info[PROPOSAL_ELECTED_TIMESTAMP_OFFSET]);                                    \
        const uint8_t governance_mode = governance_info[GOVERNANCE_MODE_OFFSET];                                                                                    \
        const uint32_t life_period = UINT32_FROM_BUF(&governance_configuration[CANDIDATE_LIFE_PERIOD_OFFSET]);                                                      \
        const uint32_t election_period = UINT32_FROM_BUF(&governance_configuration[CANDIDATE_ELECTION_PERIOD_OFFSET]);                                              \
        const uint64_t created_timestamp = UINT64_FROM_BUF(&candidate_id[CANDIDATE_CREATED_TIMESTAMP_OFFSET]);                                                      \
        uint8_t cur_status = candidate_id[CANDIDATE_STATUS_OFFSET];                                                                                                 \
        uint8_t foundation_vote_status = candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET];                                                                     \
        uint64_t status_changed_timestamp = UINT64_FROM_BUF(&candidate_id[CANDIDATE_STATUS_CHANGE_TIMESTAMP_OFFSET]);                                               \
                                                                                                                                                                    \
        status = CANDIDATE_REJECTED;                                                                                                                                \
        /* If this is piloted mode candidate we treat differently, we do not expire or reject event if there's already elected candidate. */                        \
        /* If there is a recently closed election. This voted candidate will be vetoed. (If that candidate was created before the election completion timestamp) */ \
        if ((candidate_type != PILOTED_MODE_CANDIDATE) &&                                                                                                           \
            (last_election_completed_timestamp > created_timestamp) &&                                                                                              \
            (last_election_completed_timestamp < (created_timestamp + life_period)))                                                                                \
        {                                                                                                                                                           \
            status = CANDIDATE_VETOED;                                                                                                                              \
        }                                                                                                                                                           \
        /* If the candidate is older than the life period destroy the candidate and rebate the half of staked EVRs. */                                              \
        else if ((candidate_type != PILOTED_MODE_CANDIDATE) &&                                                                                                      \
                 (cur_ledger_timestamp - created_timestamp > life_period))                                                                                          \
        {                                                                                                                                                           \
            status = CANDIDATE_EXPIRED;                                                                                                                             \
        }                                                                                                                                                           \
        /* If this is a new moment, Evaluate the votes and vote statuses. Then reset the votes for the next moment. */                                              \
        else if (cur_moment > last_vote_moment)                                                                                                                     \
        {                                                                                                                                                           \
            uint64_t election_timestamp = GET_MOMENT_START_INDEX(cur_ledger_timestamp);                                                                             \
            /* If the last vote is received before previous moment means the proposal is rejected in the next moment after last_vote_moment. */                     \
            if (cur_moment - 1 > last_vote_moment)                                                                                                                  \
            {                                                                                                                                                       \
                supported_count = 0;                                                                                                                                \
                foundation_vote_status = CANDIDATE_REJECTED;                                                                                                        \
                election_timestamp = GET_NEXT_MOMENT_START_INDEX(last_vote_timestamp + (2 * moment_size));                                                          \
            }                                                                                                                                                       \
                                                                                                                                                                    \
            /* Piloted: which means the Foundation vote determined the outcome of all Proposals */                                                                  \
            /* Co-Piloted:  which means no Proposal can succeed unless the Foundation Supports it and it fails if the Foundation opposes it. */                     \
            /* Auto-Piloted: which means the standard voting rules apply with the Foundation being treated equally with any other Participant. */                   \
            if (governance_mode == PILOTED)                                                                                                                         \
                status = foundation_vote_status;                                                                                                                    \
            else                                                                                                                                                    \
            {                                                                                                                                                       \
                const uint16_t support_average = UINT16_FROM_BUF(&governance_configuration[CANDIDATE_SUPPORT_AVERAGE_OFFSET]);                                      \
                                                                                                                                                                    \
                /* If auto-piloted standard voting rules applies for foundation. */                                                                                 \
                if (governance_mode == AUTO_PILOTED)                                                                                                                \
                {                                                                                                                                                   \
                    voter_base_count++;                                                                                                                             \
                    if (foundation_vote_status == CANDIDATE_SUPPORTED)                                                                                              \
                        supported_count++;                                                                                                                          \
                }                                                                                                                                                   \
                                                                                                                                                                    \
                if (voter_base_count > 0 && ((supported_count * 100) / voter_base_count > support_average))                                                         \
                    status = CANDIDATE_SUPPORTED;                                                                                                                   \
                                                                                                                                                                    \
                /* In co-piloted mode if foundation is not supported, we do not consider other participants' status. */                                             \
                if (governance_mode == CO_PILOTED && foundation_vote_status != CANDIDATE_SUPPORTED)                                                                 \
                    status = foundation_vote_status;                                                                                                                \
            }                                                                                                                                                       \
                                                                                                                                                                    \
            /* Update the vote status if changed. */                                                                                                                \
            if (status != cur_status)                                                                                                                               \
            {                                                                                                                                                       \
                candidate_id[CANDIDATE_STATUS_OFFSET] = status;                                                                                                     \
                cur_status = status;                                                                                                                                \
                UINT64_TO_BUF(&candidate_id[CANDIDATE_STATUS_CHANGE_TIMESTAMP_OFFSET], election_timestamp);                                                         \
                status_changed_timestamp = election_timestamp;                                                                                                      \
            }                                                                                                                                                       \
                                                                                                                                                                    \
            /* Reset the foundation vote. */                                                                                                                        \
            candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET] = (uint8_t)CANDIDATE_REJECTED;                                                                    \
            /* Update the vote counts. */                                                                                                                           \
            supported_count = 0;                                                                                                                                    \
        }                                                                                                                                                           \
                                                                                                                                                                    \
        /* If the candidate is not vetoed and expired and it fulfils the time period to get elected or vetoed. */                                                   \
        if (status != CANDIDATE_VETOED && status != CANDIDATE_EXPIRED && (cur_ledger_timestamp - status_changed_timestamp) > election_period)                       \
            status = (cur_status == CANDIDATE_SUPPORTED) ? CANDIDATE_ELECTED : CANDIDATE_VETOED;                                                                    \
    }

#endif