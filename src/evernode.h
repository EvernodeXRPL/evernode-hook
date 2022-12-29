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
#define IS_EVR(is_evr, amount_buffer, issuer_accid)                    \
    {                                                                  \
        EQUAL_20BYTES(is_evr, (amount_buffer + 8), evr_currency);      \
        if (is_evr)                                                    \
            EQUAL_20BYTES(is_evr, (amount_buffer + 28), issuer_accid); \
    }

#define MAX(num1, num2) \
    ((num1 > num2) ? num1 : num2)

#define MIN(num1, num2) \
    ((num1 > num2) ? num2 : num1)

#define CEIL(dividend, divisor) \
    ((dividend / divisor) + ((dividend % divisor) != 0))

// It gives -29 (EXPONENT_UNDERSIZED) when balance is zero. Need to read and experiment more.
#define IS_FLOAT_ZERO(float) \
    (float == 0 || float == -29)

#define EQUAL_BYTE(output, buf1, buf2) \
    output = *(uint8_t *)(buf1) == *(uint8_t *)(buf2);

#define EQUAL_2BYTES(output, buf1, buf2) \
    output = *(uint16_t *)(buf1) == *(uint16_t *)(buf2);

#define EQUAL_4BYTES(output, buf1, buf2) \
    output = *(uint32_t *)(buf1) == *(uint32_t *)(buf2);

#define EQUAL_8BYTES(output, buf1, buf2) \
    output = *(uint64_t *)(buf1) == *(uint64_t *)(buf2);

#define EQUAL_20BYTES(output, buf1, buf2)                   \
    {                                                       \
        EQUAL_8BYTES(output, buf1, buf2);                   \
        if (output)                                         \
            EQUAL_8BYTES(output, (buf1 + 8), (buf2 + 8));   \
        if (output)                                         \
            EQUAL_4BYTES(output, (buf1 + 16), (buf2 + 16)); \
    }

#define EQUAL_32BYTES(output, buf1, buf2)                   \
    {                                                       \
        EQUAL_8BYTES(output, buf1, buf2);                   \
        if (output)                                         \
            EQUAL_8BYTES(output, (buf1 + 8), (buf2 + 8));   \
        if (output)                                         \
            EQUAL_8BYTES(output, (buf1 + 16), (buf2 + 16)); \
        if (output)                                         \
            EQUAL_8BYTES(output, (buf1 + 24), (buf2 + 24)); \
    }

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

#define IS_BYTE_EMPTY(output, buf) \
    output = *(uint8_t *)(buf) == 0;

#define IS_2BYTES_EMPTY(output, buf) \
    output = *(uint16_t *)(buf) == 0;

#define IS_4BYTES_EMPTY(output, buf) \
    output = *(uint32_t *)(buf) == 0;

#define IS_8BYTES_EMPTY(output, buf) \
    output = *(uint64_t *)(buf) == 0;

#define IS_20BYTES_EMPTY(output, buf)       \
    IS_8BYTES_EMPTY(output, buf);           \
    if (output)                             \
        IS_8BYTES_EMPTY(output, (buf + 8)); \
    if (output)                             \
        IS_4BYTES_EMPTY(output, (buf + 16));

#define IS_32BYTES_EMPTY(output, buf)        \
    IS_8BYTES_EMPTY(output, buf);            \
    if (output)                              \
        IS_8BYTES_EMPTY(output, (buf + 8));  \
    if (output)                              \
        IS_8BYTES_EMPTY(output, (buf + 16)); \
    if (output)                              \
        IS_8BYTES_EMPTY(output, (buf + 24));

// Domain related comparer macros.

#define EQUAL_FORMAT_HEX(output, buf, len)                   \
    {                                                        \
        output = sizeof(FORMAT_HEX) == (len + 1);            \
        if (output)                                          \
            EQUAL_2BYTES(output, buf, FORMAT_HEX);           \
        if (output)                                          \
            EQUAL_BYTE(output, (buf + 2), (FORMAT_HEX + 2)); \
    }

#define EQUAL_FORMAT_TEXT(output, buf, len)                     \
    {                                                           \
        output = sizeof(FORMAT_TEXT) == (len + 1);              \
        if (output)                                             \
            EQUAL_8BYTES(output, buf, FORMAT_TEXT);             \
        if (output)                                             \
            EQUAL_2BYTES(output, (buf + 8), (FORMAT_TEXT + 8)); \
    }

#define EQUAL_FORMAT_BASE64(output, buf, len)                     \
    {                                                             \
        output = sizeof(FORMAT_BASE64) == (len + 1);              \
        if (output)                                               \
            EQUAL_4BYTES(output, buf, FORMAT_BASE64);             \
        if (output)                                               \
            EQUAL_2BYTES(output, (buf + 4), (FORMAT_BASE64 + 4)); \
    }

#define EQUAL_FORMAT_JSON(output, buf, len)                   \
    {                                                         \
        output = sizeof(FORMAT_JSON) == (len + 1);            \
        if (output)                                           \
            EQUAL_8BYTES(output, buf, FORMAT_JSON);           \
        if (output)                                           \
            EQUAL_BYTE(output, (buf + 8), (FORMAT_JSON + 8)); \
    }

#define EQUAL_EVR_HOST_PREFIX(output, buf)                   \
    {                                                        \
        EQUAL_4BYTES(output, buf, EVR_HOST);                 \
        if (output)                                          \
            EQUAL_2BYTES(output, (buf + 4), (EVR_HOST + 4)); \
        if (output)                                          \
            EQUAL_BYTE(output, (buf + 6), (EVR_HOST + 6));   \
    }

#define EQUAL_HOST_REG(output, buf, len)                     \
    {                                                        \
        output = sizeof(HOST_REG) == (len + 1);              \
        if (output)                                          \
            EQUAL_8BYTES(output, buf, HOST_REG);             \
        if (output)                                          \
            EQUAL_2BYTES(output, (buf + 8), (HOST_REG + 8)); \
    }

#define EQUAL_HOST_DE_REG(output, buf, len)                     \
    {                                                           \
        output = sizeof(HOST_DE_REG) == (len + 1);              \
        if (output)                                             \
            EQUAL_8BYTES(output, buf, HOST_DE_REG);             \
        if (output)                                             \
            EQUAL_4BYTES(output, (buf + 8), (HOST_DE_REG + 8)); \
    }

#define EQUAL_HOST_UPDATE_REG(output, buf, len)                     \
    {                                                               \
        output = sizeof(HOST_UPDATE_REG) == (len + 1);              \
        if (output)                                                 \
            EQUAL_8BYTES(output, buf, HOST_UPDATE_REG);             \
        if (output)                                                 \
            EQUAL_8BYTES(output, (buf + 8), (HOST_UPDATE_REG + 8)); \
    }

#define EQUAL_HEARTBEAT(output, buf, len)                     \
    {                                                         \
        output = sizeof(HEARTBEAT) == (len + 1);              \
        if (output)                                           \
            EQUAL_8BYTES(output, buf, HEARTBEAT);             \
        if (output)                                           \
            EQUAL_4BYTES(output, (buf + 8), (HEARTBEAT + 8)); \
    }

#define EQUAL_INITIALIZE(output, buf, len)                     \
    {                                                          \
        output = sizeof(INITIALIZE) == (len + 1);              \
        if (output)                                            \
            EQUAL_8BYTES(output, buf, INITIALIZE);             \
        if (output)                                            \
            EQUAL_4BYTES(output, (buf + 8), (INITIALIZE + 8)); \
        if (output)                                            \
            EQUAL_BYTE(output, (buf + 12), (INITIALIZE + 12)); \
    }

#define EQUAL_HOST_POST_DEREG(output, buf, len)                     \
    {                                                               \
        output = sizeof(HOST_POST_DEREG) == (len + 1);              \
        if (output)                                                 \
            EQUAL_8BYTES(output, buf, HOST_POST_DEREG);             \
        if (output)                                                 \
            EQUAL_8BYTES(output, (buf + 8), (HOST_POST_DEREG + 8)); \
    }

#define EQUAL_DEAD_HOST_PRUNE(output, buf, len)                     \
    {                                                               \
        output = sizeof(DEAD_HOST_PRUNE) == (len + 1);              \
        if (output)                                                 \
            EQUAL_8BYTES(output, buf, DEAD_HOST_PRUNE);             \
        if (output)                                                 \
            EQUAL_8BYTES(output, (buf + 8), (DEAD_HOST_PRUNE + 8)); \
    }

#define EQUAL_HOST_TRANSFER(output, buf, len)                     \
    {                                                             \
        output = sizeof(HOST_TRANSFER) == (len + 1);              \
        if (output)                                               \
            EQUAL_8BYTES(output, buf, HOST_TRANSFER);             \
        if (output)                                               \
            EQUAL_2BYTES(output, (buf + 8), (HOST_TRANSFER + 8)); \
        if (output)                                               \
            EQUAL_BYTE(output, (buf + 10), (HOST_TRANSFER + 10)); \
    }

#define EQUAL_HOST_REBATE(output, buf, len)                     \
    {                                                           \
        output = sizeof(HOST_REBATE) == (len + 1);              \
        if (output)                                             \
            EQUAL_8BYTES(output, buf, HOST_REBATE);             \
        if (output)                                             \
            EQUAL_4BYTES(output, (buf + 8), (HOST_REBATE + 8)); \
        if (output)                                             \
            EQUAL_BYTE(output, (buf + 12), (HOST_REBATE + 12)); \
    }

#define EQUAL_HOOK_UPDATE(output, buf, len)                     \
    {                                                           \
        output = sizeof(HOOK_UPDATE) == (len + 1);              \
        if (output)                                             \
            EQUAL_8BYTES(output, buf, HOOK_UPDATE);             \
        if (output)                                             \
            EQUAL_4BYTES(output, (buf + 8), (HOOK_UPDATE + 8)); \
        if (output)                                             \
            EQUAL_BYTE(output, (buf + 12), (HOOK_UPDATE + 12)); \
    }

#define EQUAL_HOST_REGISTRY_REF(output, buf, len)                       \
    {                                                                   \
        output = sizeof(HOST_REGISTRY_REF) == (len + 1);                \
        if (output)                                                     \
            EQUAL_8BYTES(output, buf, HOST_REGISTRY_REF);               \
        if (output)                                                     \
            EQUAL_8BYTES(output, (buf + 8), (HOST_REGISTRY_REF + 8));   \
        if (output)                                                     \
            EQUAL_2BYTES(output, (buf + 16), (HOST_REGISTRY_REF + 16)); \
    }

// Domain related copy macros.

#define COPY_EVR_HOST_PREFIX(buf, spos)            \
    COPY_4BYTES((buf + spos), EVR_HOST);           \
    COPY_2BYTES((buf + spos + 4), (EVR_HOST + 4)); \
    COPY_BYTE((buf + spos + 6), (EVR_HOST + 6));

#define COPY_MOMENT_TRANSIT_INFO(lhsbuf, rhsbuf) \
    COPY_8BYTES(lhsbuf, rhsbuf);                 \
    COPY_2BYTES((lhsbuf + 8), (rhsbuf + 8));     \
    COPY_BYTE((lhsbuf + 10), (rhsbuf + 10));

#define COPY_DESCRIPTION(lhsbuf, rhsbuf)       \
    COPY_8BYTES(lhsbuf, rhsbuf);               \
    COPY_8BYTES((lhsbuf + 8), (rhsbuf + 8));   \
    COPY_8BYTES((lhsbuf + 16), (rhsbuf + 16)); \
    COPY_2BYTES((lhsbuf + 24), (rhsbuf + 24));

#define COPY_REG_NFT_URI(lhsbuf, rhsbuf)       \
    COPY_32BYTES(lhsbuf, rhsbuf);              \
    COPY_4BYTES((lhsbuf + 32), (rhsbuf + 32)); \
    COPY_2BYTES((lhsbuf + 36), (rhsbuf + 36)); \
    COPY_BYTE((lhsbuf + 38), (rhsbuf + 38));

// Domain related clear macros.

#define CLEAR_MOMENT_TRANSIT_INFO(buf, spos) \
    CLEAR_8BYTES(buf);                       \
    CLEAR_2BYTES((buf + 8));                 \
    CLEAR_BYTE((buf + 2))

// Domain related empty check macros.

#define IS_MOMENT_TRANSIT_INFO_EMPTY(output, buf) \
    IS_8BYTES_EMPTY(output, buf);                 \
    if (output)                                   \
        IS_2BYTES_EMPTY(output, (buf + 8));       \
    if (output)                                   \
        IS_BYTE_EMPTY(output, (buf + 10));

#define IS_VERSION_EMPTY(output, buf) \
    IS_2BYTES_EMPTY(output, buf);     \
    if (output)                       \
        IS_BYTE_EMPTY(output, (buf + 2));

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

#define GET_CONF_VALUE(value, key, error_buf)                    \
    {                                                            \
        uint8_t size = sizeof(value);                            \
        uint8_t value_buf[size];                                 \
        int64_t state_res = state(SBUF(value_buf), SBUF(key));   \
        if (state_res < 0)                                       \
            rollback(SBUF(error_buf), 1);                        \
        switch (size)                                            \
        {                                                        \
        case 2:                                                  \
            value = UINT16_FROM_BUF(value_buf);                  \
            break;                                               \
        case 4:                                                  \
            value = UINT32_FROM_BUF(value_buf);                  \
            break;                                               \
        case 8:                                                  \
            value = UINT64_FROM_BUF(value_buf);                  \
            break;                                               \
        default:                                                 \
            rollback(SBUF("Evernode: Invalid state value."), 1); \
            break;                                               \
        }                                                        \
    }

#define GET_FLOAT_CONF_VALUE(value, def_mentissa, def_exponent, key, error_buf) \
    {                                                                           \
        uint8_t value_buf[8];                                                   \
        int64_t state_res = state(SBUF(value_buf), SBUF(key));                  \
        if (state_res == DOESNT_EXIST)                                          \
        {                                                                       \
            value = float_set(def_exponent, def_mentissa);                      \
            INT64_TO_BUF(value_buf, value);                                     \
        }                                                                       \
        else                                                                    \
            value = INT64_FROM_BUF(value_buf);                                  \
                                                                                \
        if (state_res == DOESNT_EXIST)                                          \
        {                                                                       \
            if (state_set(SBUF(value_buf), SBUF(key)) < 0)                      \
                rollback(SBUF(error_buf), 1);                                   \
        }                                                                       \
    }

// If host count state does not exist, set host count to 0.
#define GET_HOST_COUNT(host_count)                                             \
    {                                                                          \
        uint8_t host_count_buf[4] = {0};                                       \
        host_count = 0;                                                        \
        if (state(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) != DOESNT_EXIST) \
            host_count = UINT32_FROM_BUF(host_count_buf);                      \
    }

#define SET_HOST_COUNT(host_count)                                                      \
    {                                                                                   \
        uint8_t host_count_buf[4] = {0};                                                \
        UINT32_TO_BUF(host_count_buf, host_count);                                      \
        if (state_set(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) < 0)                  \
            rollback(SBUF("Evernode: Could not set default state for host count."), 1); \
    }

#define GENERATE_NFT_TOKEN_ID_GUARD(token_id, tflag, transaction_fee, accid, taxon, token_seq, n) \
    {                                                                                             \
        UINT16_TO_BUF(token_id, tflag);                                                           \
        UINT16_TO_BUF((token_id + 2), transaction_fee);                                           \
        COPY_20BYTES((token_id + 4), accid);                                                      \
        UINT32_TO_BUF((token_id + 24), (taxon ^ ((NFT_TAXON_M * token_seq) + NFT_TAXON_C)));      \
        UINT32_TO_BUF((token_id + 28), token_seq);                                                \
    }

#define GENERATE_NFT_TOKEN_ID(token_id, tflag, transaction_fee, accid, taxon, token_seq) \
    GENERATE_NFT_TOKEN_ID_GUARD(token_id, tflag, transaction_fee, accid, taxon, token_seq, 1)

#define GET_MOMENT(moment, idx)                                                                            \
    {                                                                                                      \
        uint16_t moment_size;                                                                              \
        GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");             \
        uint8_t moment_base_info[MOMENT_BASE_INFO_VAL_SIZE];                                               \
        if (state(moment_base_info, MOMENT_BASE_INFO_VAL_SIZE, SBUF(STK_MOMENT_BASE_INFO)) < 0)            \
            rollback(SBUF("Evernode: Could not get moment base info state."), 1);                          \
        uint64_t moment_base_idx = UINT64_FROM_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);           \
        uint32_t prev_transition_moment = UINT32_FROM_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]); \
        uint64_t relative_n = (idx - moment_base_idx) / moment_size;                                       \
        moment = prev_transition_moment + relative_n;                                                      \
    }

#define GET_MOMENT_END_INDEX(moment_end_idx, idx)                                                \
    {                                                                                            \
        uint16_t moment_size;                                                                    \
        GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");   \
        uint8_t moment_base_info[MOMENT_BASE_INFO_VAL_SIZE];                                     \
        if (state(moment_base_info, MOMENT_BASE_INFO_VAL_SIZE, SBUF(STK_MOMENT_BASE_INFO)) < 0)  \
            rollback(SBUF("Evernode: Could not get moment base info state."), 1);                \
        uint64_t moment_base_idx = UINT64_FROM_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET]); \
        uint64_t relative_n = (idx - moment_base_idx) / moment_size;                             \
        moment_end_idx = moment_base_idx + ((relative_n + 1) * moment_size);                     \
    }

#define IS_REG_NFT_EXIST(account, nft_id, nft_keylet, nft_loc_idx, nft_exists)                                                                     \
    {                                                                                                                                              \
        nft_exists = 0;                                                                                                                            \
        COPY_20BYTES((nft_keylet + 2), account);                                                                                                   \
        int64_t nft_slot = slot_set(nft_keylet, 34, 0);                                                                                            \
        if (nft_slot < 0)                                                                                                                          \
            rollback(SBUF("Evernode: Could not set ledger nft keylet in slot"), 10);                                                               \
                                                                                                                                                   \
        nft_slot = slot_subfield(nft_slot, sfNFTokens, 0);                                                                                         \
        if (nft_slot < 0)                                                                                                                          \
            rollback(SBUF("Evernode: Could not find sfNFTokens on ledger nft keylet"), 1);                                                         \
                                                                                                                                                   \
        nft_slot = slot_subarray(nft_slot, nft_loc_idx, 0);                                                                                        \
        if (nft_slot >= 0)                                                                                                                         \
        {                                                                                                                                          \
            uint8_t cur_id[NFT_TOKEN_ID_SIZE] = {0};                                                                                               \
            int64_t cur_slot = slot_subfield(nft_slot, sfNFTokenID, 0);                                                                            \
            if (cur_slot >= 0 && slot(SBUF(cur_id), cur_slot) == NFT_TOKEN_ID_SIZE)                                                                \
            {                                                                                                                                      \
                COPY_20BYTES((nft_id + 4), hook_accid); /*Issuer of the NFT should be the registry contract.*/                                     \
                EQUAL_32BYTES(nft_exists, cur_id, nft_id);                                                                                         \
                if (nft_exists)                                                                                                                    \
                {                                                                                                                                  \
                    uint8_t uri_read_buf[258];                                                                                                     \
                    cur_slot = slot_subfield(nft_slot, sfURI, 0);                                                                                  \
                    int64_t uri_read_len = slot(SBUF(uri_read_buf), cur_slot);                                                                     \
                    int64_t nft_uri_len = (uri_read_len >= 195) ? 193 + ((uri_read_buf[0] - 193) * 256) + uri_read_buf[1] : uri_read_buf[0];       \
                    if (nft_uri_len == REG_NFT_URI_SIZE)                                                                                           \
                    {                                                                                                                              \
                        EQUAL_EVR_HOST_PREFIX(nft_exists, (uri_read_buf + (uri_read_len >= 195 ? 2 : 1))); /*NFT URI should start with 'evrhost'*/ \
                    }                                                                                                                              \
                    else                                                                                                                           \
                    {                                                                                                                              \
                        nft_exists = 0;                                                                                                            \
                    }                                                                                                                              \
                }                                                                                                                                  \
            }                                                                                                                                      \
        }                                                                                                                                          \
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