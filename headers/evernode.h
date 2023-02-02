#ifndef EVERNODE_INCLUDED
#define EVERNODE_INCLUDED 1

#include "constants.h"
#include "transactions.h"

/****** Field and Type codes ******/
#define ARRAY 0xF0U
#define OBJECT 0xE0U
#define MEMOS 0X9
#define MEMO 0XA
#define HOOKS 0XB
#define HOOK 0XE
#define END 1
#define MEMO_TYPE 0xC
#define MEMO_DATA 0xD
#define MEMO_FORMAT 0xE

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

#define EQUAL_HOST_REGISTRY_REF(buf, len)                  \
    (sizeof(HOST_REGISTRY_REF) == (len + 1) &&             \
     BUFFER_EQUAL_8(buf, HOST_REGISTRY_REF) &&             \
     BUFFER_EQUAL_8((buf + 8), (HOST_REGISTRY_REF + 8)) && \
     BUFFER_EQUAL_2((buf + 16), (HOST_REGISTRY_REF + 16)))

// Provide m >= 1 to indicate in which code line macro will hit.
// Provide n >= 1 to indicate how many times the macro will be hit on the line of code.
// e.g. if it is in a loop that loops 10 times n = 10
// If it is used 3 times inside a macro use m = 1,2,3
// We need to dump the iou amount into a buffer.
// by supplying -1 as the fieldcode we tell float_sto not to prefix an actual STO header on the field.
#define SET_AMOUNT_OUT_GUARDM(amt_out, token, issuer, amount, n, m)              \
    {                                                                            \
        uint8_t currency[20] = GET_TOKEN_CURRENCY(token);                        \
        if (float_sto(amt_out, 48, currency, 20, issuer, 20, amount, -1) < 0)    \
            rollback(SBUF("Evernode: Could not dump token amount into sto"), 1); \
        COPY_20BYTES((amt_out + 8), currency);                                   \
        COPY_20BYTES((amt_out + 28), issuer);                                    \
        if (amount == 0)                                                         \
            amt_out[0] = amt_out[0] & 0b10111111; /* Set the sign bit to 0.*/    \
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

#define GET_MOMENT(moment, idx)                                                                                      \
    {                                                                                                                \
        uint16_t moment_size;                                                                                        \
        GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");                       \
        uint8_t moment_base_info[MOMENT_BASE_INFO_VAL_SIZE];                                                         \
        if (state_foreign(moment_base_info, MOMENT_BASE_INFO_VAL_SIZE, SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF) < 0) \
            rollback(SBUF("Evernode: Could not get moment base info state."), 1);                                    \
        uint64_t moment_base_idx = UINT64_FROM_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);                     \
        uint32_t prev_transition_moment = UINT32_FROM_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]);           \
        uint64_t relative_n = (idx - moment_base_idx) / moment_size;                                                 \
        moment = prev_transition_moment + relative_n;                                                                \
    }

#define GET_MOMENT_END_INDEX(moment_end_idx, idx)                                                                    \
    {                                                                                                                \
        uint16_t moment_size;                                                                                        \
        GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");                       \
        uint8_t moment_base_info[MOMENT_BASE_INFO_VAL_SIZE];                                                         \
        if (state_foreign(moment_base_info, MOMENT_BASE_INFO_VAL_SIZE, SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF) < 0) \
            rollback(SBUF("Evernode: Could not get moment base info state."), 1);                                    \
        uint64_t moment_base_idx = UINT64_FROM_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);                     \
        uint64_t relative_n = (idx - moment_base_idx) / moment_size;                                                 \
        moment_end_idx = moment_base_idx + ((relative_n + 1) * moment_size);                                         \
    }

#define IS_REG_NFT_EXIST(account, nft_minter, nft_id, nft_keylet, nft_loc_idx, nft_exists)                                                   \
    {                                                                                                                                        \
        nft_exists = 0;                                                                                                                      \
        COPY_20BYTES((nft_keylet + 2), account);                                                                                             \
        int64_t nft_slot = slot_set(nft_keylet, 34, 0);                                                                                      \
        if (nft_slot < 0)                                                                                                                    \
            rollback(SBUF("Evernode: Could not set ledger nft keylet in slot"), 10);                                                         \
                                                                                                                                             \
        nft_slot = slot_subfield(nft_slot, sfNFTokens, 0);                                                                                   \
        if (nft_slot < 0)                                                                                                                    \
            rollback(SBUF("Evernode: Could not find sfNFTokens on ledger nft keylet"), 1);                                                   \
                                                                                                                                             \
        nft_slot = slot_subarray(nft_slot, nft_loc_idx, 0);                                                                                  \
        if (nft_slot >= 0)                                                                                                                   \
        {                                                                                                                                    \
            uint8_t cur_id[NFT_TOKEN_ID_SIZE] = {0};                                                                                         \
            int64_t cur_slot = slot_subfield(nft_slot, sfNFTokenID, 0);                                                                      \
            if (cur_slot >= 0 && slot(SBUF(cur_id), cur_slot) == NFT_TOKEN_ID_SIZE)                                                          \
            {                                                                                                                                \
                COPY_20BYTES((nft_id + 4), nft_minter); /*Issuer of the NFT should be the registry contract.*/                               \
                if (BUFFER_EQUAL_32(cur_id, nft_id))                                                                                         \
                {                                                                                                                            \
                    uint8_t uri_read_buf[258];                                                                                               \
                    cur_slot = slot_subfield(nft_slot, sfURI, 0);                                                                            \
                    int64_t uri_read_len = slot(SBUF(uri_read_buf), cur_slot);                                                               \
                    int64_t nft_uri_len = (uri_read_len >= 195) ? 193 + ((uri_read_buf[0] - 193) * 256) + uri_read_buf[1] : uri_read_buf[0]; \
                    /*NFT URI should start with 'evrhost'*/                                                                                  \
                    nft_exists = (nft_uri_len == REG_NFT_URI_SIZE && EQUAL_EVR_HOST_PREFIX((uri_read_buf + (uri_read_len >= 195 ? 2 : 1)))); \
                }                                                                                                                            \
            }                                                                                                                                \
        }                                                                                                                                    \
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

#define PREPARE_EPOCH_REWARD_INFO(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, moment_size, is_heartbeat, reward_pool_amount_ref, reward_amount_ref) \
    {                                                                                                                                                                                             \
        const uint8_t epoch = reward_info[EPOCH_OFFSET];                                                                                                                                          \
        uint32_t reward_quota;                                                                                                                                                                    \
        GET_EPOCH_REWARD_QUOTA(epoch, first_epoch_reward_quota, reward_quota);                                                                                                                    \
        uint32_t prev_moment_active_host_count = UINT32_FROM_BUF(&reward_info[PREV_MOMENT_ACTIVE_HOST_COUNT_OFFSET]);                                                                             \
        const uint32_t cur_moment_active_host_count = UINT32_FROM_BUF(&reward_info[CUR_MOMENT_ACTIVE_HOST_COUNT_OFFSET]);                                                                         \
        const uint8_t *pool_ptr = &reward_info[EPOCH_POOL_OFFSET];                                                                                                                                \
        reward_pool_amount_ref = INT64_FROM_BUF(pool_ptr);                                                                                                                                        \
        const uint32_t saved_moment = UINT32_FROM_BUF(&reward_info[SAVED_MOMENT_OFFSET]);                                                                                                         \
        uint32_t cur_moment;                                                                                                                                                                      \
        GET_MOMENT(cur_moment, cur_idx);                                                                                                                                                          \
        /* If this is a new moment, update the host counts. */                                                                                                                                    \
        if (saved_moment != cur_moment)                                                                                                                                                           \
        {                                                                                                                                                                                         \
            /* Remove previous moment data and move current moment data to previous moment. */                                                                                                    \
            UINT32_TO_BUF(&reward_info[SAVED_MOMENT_OFFSET], cur_moment);                                                                                                                         \
            /* If the saved moment is not cur_moment - 1, We've missed some moments. Means there was no heartbeat received in previous moment. */                                                 \
            prev_moment_active_host_count = ((saved_moment == cur_moment - 1) ? cur_moment_active_host_count : 0);                                                                                \
            UINT32_TO_BUF(&reward_info[PREV_MOMENT_ACTIVE_HOST_COUNT_OFFSET], prev_moment_active_host_count);                                                                                     \
            /* If the macro is called from heartbeat intialte cur moment active host count as 1. */                                                                                               \
            UINT32_TO_BUF(&reward_info[CUR_MOMENT_ACTIVE_HOST_COUNT_OFFSET], (is_heartbeat ? 1 : 0));                                                                                             \
        }                                                                                                                                                                                         \
        /* If the macro is called from heartbeat increase cur moment active host count by 1. */                                                                                                   \
        else if (is_heartbeat)                                                                                                                                                                    \
        {                                                                                                                                                                                         \
            UINT32_TO_BUF(&reward_info[CUR_MOMENT_ACTIVE_HOST_COUNT_OFFSET], (cur_moment_active_host_count + 1));                                                                                 \
        }                                                                                                                                                                                         \
        /* Reward pool amount is less than the reward quota for the moment, Increment the epoch. And add the remaining to the next epoch pool. */                                                 \
        if (float_compare(reward_pool_amount_ref, float_set(0, reward_quota), COMPARE_LESS) == 1)                                                                                                 \
        {                                                                                                                                                                                         \
            /* If the current epoch is < epoch count increment otherwise skip. */                                                                                                                 \
            if (epoch < epoch_count)                                                                                                                                                              \
            {                                                                                                                                                                                     \
                reward_pool_amount_ref = float_sum(float_set(0, epoch_reward_amount), reward_pool_amount_ref);                                                                                    \
                INT64_TO_BUF(pool_ptr, reward_pool_amount_ref);                                                                                                                                   \
                reward_info[EPOCH_OFFSET] = epoch + 1;                                                                                                                                            \
                /* When epoch incremented by 1, reward quota halves. */                                                                                                                           \
                reward_quota = reward_quota / 2;                                                                                                                                                  \
            }                                                                                                                                                                                     \
            else                                                                                                                                                                                  \
            {                                                                                                                                                                                     \
                reward_quota = 0;                                                                                                                                                                 \
            }                                                                                                                                                                                     \
        }                                                                                                                                                                                         \
        /* Calculate the reward quota for the current epoch. */                                                                                                                                   \
        /* Use float division only if modulo is not zero to reduce floating point complications. */                                                                                               \
        if (prev_moment_active_host_count == 0)                                                                                                                                                   \
            reward_amount_ref = float_set(0, 0);                                                                                                                                                  \
        else if (reward_quota % prev_moment_active_host_count == 0)                                                                                                                               \
            reward_amount_ref = float_set(0, (reward_quota / prev_moment_active_host_count));                                                                                                     \
        else                                                                                                                                                                                      \
            reward_amount_ref = float_divide(float_set(0, reward_quota), float_set(0, prev_moment_active_host_count));                                                                            \
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

#endif