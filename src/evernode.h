#ifndef EVERNODE_INCLUDED
#define EVERNODE_INCLUDED 1

#include "constants.h"

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
#define SET_AMOUNT_OUT_GUARDM(amt_out, token, issuer, amount, n, m)               \
    {                                                                             \
        uint8_t currency[20] = GET_TOKEN_CURRENCY(token);                         \
        if (float_sto(SBUF(amt_out), SBUF(currency), issuer, 20, amount, -1) < 0) \
            rollback(SBUF("Evernode: Could not dump token amount into sto"), 1);  \
        COPY_20BYTES((amt_out + 8), currency);                                    \
        COPY_20BYTES((amt_out + 28), issuer);                                     \
        if (amount == 0)                                                          \
            amt_out[0] = amt_out[0] & 0b10111111; /* Set the sign bit to 0.*/     \
    }

#define SET_AMOUNT_OUT_GUARD(amt_out, token, issuer, amount, n) \
    SET_AMOUNT_OUT_GUARDM(amt_out, token, issuer, amount, n, 1)

#define SET_AMOUNT_OUT(amt_out, token, issuer, amount) \
    SET_AMOUNT_OUT_GUARDM(amt_out, token, issuer, amount, 1, 1)

#define GET_MEMO(index, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len) \
    {                                                                                                                         \
        /* since our memos are in a buffer inside the hook (as opposed to being a slot) we use the sto api with it            \
        the sto apis probe into a serialized object returning offsets and lengths of subfields or array entries */            \
        int64_t memo_lookup = sto_subarray(memos, memos_len, index);                                                          \
        memo_ptr = SUB_OFFSET(memo_lookup) + memos;                                                                           \
        memo_len = SUB_LENGTH(memo_lookup);                                                                                   \
        /* memos are nested inside an actual memo object, so we need to subfield                                              \
        / equivalently in JSON this would look like memo_array[i]["Memo"] */                                                  \
        memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemo);                                                               \
        memo_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;                                                                        \
        memo_len = SUB_LENGTH(memo_lookup);                                                                                   \
        if (memo_lookup < 0)                                                                                                  \
            accept(SBUF("Evernode: Incoming txn had a blank sfMemos."), 1);                                                   \
        memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemoType);                                                           \
        type_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;                                                                        \
        type_len = SUB_LENGTH(memo_lookup);                                                                                   \
        /* trace(SBUF("type in hex: "), type_ptr, type_len, 1); */                                                            \
        memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemoFormat);                                                         \
        format_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;                                                                      \
        format_len = SUB_LENGTH(memo_lookup);                                                                                 \
        /* trace(SBUF("format in hex: "), format_ptr, format_len, 1); */                                                      \
        memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemoData);                                                           \
        data_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;                                                                        \
        data_len = SUB_LENGTH(memo_lookup);                                                                                   \
        /* trace(SBUF("data in hex: "), data_ptr, data_len, 1); // Text data is in hex format. */                             \
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

enum LedgerEntryType
{
    ltNFTOKEN_PAGE = 0x0050
};

const uint8_t page_mask[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

#define GET_NFT(account, nft_id, nft_exists, nft_issuer, nft_uri, nft_uri_len, nft_taxon, nft_flags, nft_tffee, nft_seq)     \
    {                                                                                                                        \
        nft_exists = 0;                                                                                                      \
        uint8_t lo_keylet[34];                                                                                               \
        uint8_t buf[32] = {0};                                                                                               \
        COPY_20BYTES(buf, account);                                                                                          \
        lo_keylet[0] = (ltNFTOKEN_PAGE >> 8) & 0xFFU;                                                                        \
        lo_keylet[1] = (ltNFTOKEN_PAGE >> 0) & 0xFFU;                                                                        \
        COPY_32BYTES((lo_keylet + 2), buf);                                                                                  \
                                                                                                                             \
        uint8_t id_keylet[34] = {0};                                                                                         \
        id_keylet[0] = (ltNFTOKEN_PAGE >> 8) & 0xFFU;                                                                        \
        id_keylet[1] = (ltNFTOKEN_PAGE >> 0) & 0xFFU;                                                                        \
        for (int i = 0; GUARDM(32, 3), i < 32; ++i)                                                                          \
            id_keylet[2 + i] = (lo_keylet[2 + i] & ~page_mask[i]) + (nft_id[i] & page_mask[i]);                              \
                                                                                                                             \
        uint8_t hi_keylet[34];                                                                                               \
        uint8_t id[32];                                                                                                      \
        COPY_20BYTES(id, account);                                                                                           \
        COPY_8BYTES((id + ACCOUNT_ID_SIZE), (page_mask + ACCOUNT_ID_SIZE));                                                  \
        COPY_4BYTES((id + ACCOUNT_ID_SIZE + 8), (page_mask + ACCOUNT_ID_SIZE + 8));                                          \
        hi_keylet[0] = (ltNFTOKEN_PAGE >> 8) & 0xFFU;                                                                        \
        hi_keylet[1] = (ltNFTOKEN_PAGE >> 0) & 0xFFU;                                                                        \
        COPY_32BYTES((hi_keylet + 2), id);                                                                                   \
                                                                                                                             \
        uint8_t nft_keylet[34];                                                                                              \
        if (ledger_keylet(SBUF(nft_keylet), SBUF(id_keylet), SBUF(hi_keylet)) != 34)                                         \
            rollback(SBUF("Evernode: Could not generate the ledger nft keylet."), 10);                                       \
                                                                                                                             \
        int64_t nfts_slot = slot_set(SBUF(nft_keylet), 0);                                                                   \
        if (nfts_slot < 0)                                                                                                   \
            rollback(SBUF("Evernode: Could not set ledger nft keylet in slot"), 10);                                         \
                                                                                                                             \
        nfts_slot = slot_subfield(nfts_slot, sfNFTokens, 0);                                                                 \
        if (nfts_slot < 0)                                                                                                   \
            rollback(SBUF("Evernode: Could not find sfNFTokens on ledger nft keylet"), 1);                                   \
                                                                                                                             \
        uint8_t cur_id[NFT_TOKEN_ID_SIZE] = {0};                                                                             \
        uint8_t uri_read_buf[258];                                                                                           \
        int64_t uri_read_len;                                                                                                \
        for (int i = 0; GUARDM(32, 7), i < 32; ++i)                                                                          \
        {                                                                                                                    \
            int64_t nft_slot = slot_subarray(nfts_slot, i, 0);                                                               \
            if (nft_slot >= 0)                                                                                               \
            {                                                                                                                \
                int64_t id_slot = slot_subfield(nft_slot, sfNFTokenID, 0);                                                   \
                if (id_slot >= 0 && slot(SBUF(cur_id), id_slot) == NFT_TOKEN_ID_SIZE)                                        \
                {                                                                                                            \
                    int equal = 0;                                                                                           \
                    EQUAL_32BYTES(equal, cur_id, nft_id);                                                                    \
                    if (equal)                                                                                               \
                    {                                                                                                        \
                        int64_t uri_slot = slot_subfield(nft_slot, sfURI, 0);                                                \
                        uri_read_len = slot(SBUF(uri_read_buf), uri_slot);                                                   \
                        nft_exists = 1;                                                                                      \
                        break;                                                                                               \
                    }                                                                                                        \
                }                                                                                                            \
            }                                                                                                                \
        }                                                                                                                    \
        if (nft_exists)                                                                                                      \
        {                                                                                                                    \
            nft_uri_len = (uri_read_len >= 195) ? 193 + ((uri_read_buf[0] - 193) * 256) + uri_read_buf[1] : uri_read_buf[0]; \
            COPY_REG_NFT_URI(nft_uri, (uri_read_buf + (uri_read_len >= 195 ? 2 : 1)));                                       \
            nft_flags = UINT16_FROM_BUF(cur_id);                                                                             \
            nft_tffee = UINT16_FROM_BUF((cur_id + 2));                                                                       \
            COPY_20BYTES(nft_issuer, (cur_id + 4));                                                                          \
            uint32_t taxon = UINT32_FROM_BUF((cur_id + 24));                                                                 \
            nft_seq = UINT32_FROM_BUF((cur_id + 28));                                                                        \
            nft_taxon = taxon ^ ((NFT_TAXON_M * nft_seq) + NFT_TAXON_C);                                                     \
        }                                                                                                                    \
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

/**************************************************************************/
/***************************NFT related MACROS*****************************/
/**************************************************************************/

#define ENCODE_TF_SIZE 3
#define ENCODE_TF(buf_out, to)           \
    {                                    \
        uint8_t uto = to;                \
        buf_out[0] = 0x14U;              \
        buf_out[1] = (uto >> 8) & 0xFFU; \
        buf_out[2] = (uto >> 0) & 0xFFU; \
        buf_out += ENCODE_TF_SIZE;       \
    }

#define _01_04_ENCODE_TF(buf_out, to) \
    ENCODE_TF(buf_out, to);

#define ENCODE_HOOKHASH_SIZE 34
#define ENCODE_HOOKHASH(buf_out, hash_ptr)                          \
    {                                                               \
        buf_out[0] = 0x50U + 0U;                                    \
        buf_out[1] = 0x1FU;                                         \
        *(uint64_t *)(buf_out + 2) = *(uint64_t *)(hash_ptr + 0);   \
        *(uint64_t *)(buf_out + 10) = *(uint64_t *)(hash_ptr + 8);  \
        *(uint64_t *)(buf_out + 18) = *(uint64_t *)(hash_ptr + 16); \
        *(uint64_t *)(buf_out + 26) = *(uint64_t *)(hash_ptr + 24); \
        buf_out += ENCODE_HOOKHASH_SIZE;                            \
    }

#define _05_31_ENCODE_HOOKHASH(buf_out, hash_ptr) \
    ENCODE_HOOKHASH(buf_out, hash_ptr);

#define ENCODE_DELETEHOOK_SIZE 2
#define ENCODE_DELETEHOOK(buf_out)         \
    {                                      \
        buf_out[0] = 0x70U + 0xBU;         \
        buf_out[1] = 0x00U;                \
        buf_out += ENCODE_DELETEHOOK_SIZE; \
    }

#define _07_11_ENCODE_DELETEHOOK(buf_out) \
    ENCODE_DELETEHOOK(buf_out);

#define ENCODE_NAMESPACE_SIZE 34
#define ENCODE_NAMESPACE(buf_out, namespace)                         \
    {                                                                \
        buf_out[0] = 0x50U + 0U;                                     \
        buf_out[1] = 0x20U;                                          \
        *(uint64_t *)(buf_out + 2) = *(uint64_t *)(namespace + 0);   \
        *(uint64_t *)(buf_out + 10) = *(uint64_t *)(namespace + 8);  \
        *(uint64_t *)(buf_out + 18) = *(uint64_t *)(namespace + 16); \
        *(uint64_t *)(buf_out + 26) = *(uint64_t *)(namespace + 24); \
        buf_out += ENCODE_NAMESPACE_SIZE;                            \
    }

#define _05_32_ENCODE_NAMESPACE(buf_out, namespace) \
    ENCODE_NAMESPACE(buf_out, namespace);

#define ENCODE_TXON_SIZE 6U
#define ENCODE_TXON(buf_out, tf) \
    ENCODE_UINT32_UNCOMMON(buf_out, tf, 0x2A);
#define _02_42_ENCODE_TXON(buf_out, tf) \
    ENCODE_TXON(buf_out, tf);

#define ENCODE_URI(buf_out, uri, uri_len)     \
    {                                         \
        buf_out[0] = 0x75U;                   \
        buf_out[1] = uri_len;                 \
        COPY_REG_NFT_URI((buf_out + 2), uri); \
        buf_out += REG_NFT_URI_SIZE + 2;      \
    }
#define _07_05_ENCODE_URI(buf_out, uri, uri_len) \
    ENCODE_URI(buf_out, uri, uri_len);

#define ENCODE_HASH_SIZE 33
#define ENCODE_HASH(buf_out, hash, field)                       \
    {                                                           \
        uint8_t uf = field;                                     \
        buf_out[0] = 0x50U + (uf & 0x0FU);                      \
        *(uint64_t *)(buf_out + 1) = *(uint64_t *)(hash + 0);   \
        *(uint64_t *)(buf_out + 9) = *(uint64_t *)(hash + 8);   \
        *(uint64_t *)(buf_out + 17) = *(uint64_t *)(hash + 16); \
        *(uint64_t *)(buf_out + 25) = *(uint64_t *)(hash + 24); \
        buf_out += ENCODE_HASH_SIZE;                            \
    }
#define _05_10_ENCODE_EMIT_PARENT_TXN_ID(buf_out, txn_id) \
    ENCODE_HASH(buf_out, txn_id, 0XAU);

#define ENCODE_EXPIRATION_SIZE 5
#define ENCODE_EXPIRATION(buf_out, exp) \
    ENCODE_UINT32_COMMON(buf_out, exp, 0xAU);
#define _02_10_ENCODE_EXPIRATION_MAX(buf_out) \
    ENCODE_EXPIRATION(buf_out, 0xFFFFFFFFU);

/**************************************************************************/
/**************************MEMO related MACROS*****************************/
/**************************************************************************/

#define ENCODE_FIELDS_SIZE 1U
#define ENCODE_FIELDS(buf_out, type, field) \
    {                                       \
        uint8_t uf = field;                 \
        buf_out[0] = type + (uf & 0x0FU);   \
        buf_out += ENCODE_FIELDS_SIZE;      \
    }

// Memo data is truncated at 32bytes, because maximum memo hook would send in a transaction id.
#define ENCODE_STI_VL_COMMON_GUARDM(buf_out, data, data_len, field, n, m)     \
    {                                                                         \
        uint8_t *ptr = (uint8_t *)&data;                                      \
        uint8_t uf = field;                                                   \
        buf_out[0] = 0x70U + (uf & 0x0FU);                                    \
        const int len = MIN(data_len, 32);                                    \
        if (len <= 192) /*Data length is represented with 2 bytes if (>192)*/ \
        {                                                                     \
            buf_out[1] = len;                                                 \
            buf_out += 2;                                                     \
        }                                                                     \
        else                                                                  \
        {                                                                     \
            buf_out[0] = 0x70U + (uf & 0x0FU);                                \
            buf_out[1] = ((len - 193) / 256) + 193;                           \
            buf_out[2] = len - (((buf_out[1] - 193) * 256) + 193);            \
            buf_out += 3;                                                     \
        }                                                                     \
        int pos = 0;                                                          \
        if (pos + 32 <= len)                                                  \
        {                                                                     \
            COPY_32BYTES(buf_out, (ptr + pos));                               \
            pos += 32;                                                        \
        }                                                                     \
        if (pos + 16 <= len)                                                  \
        {                                                                     \
            COPY_16BYTES(buf_out, (ptr + pos));                               \
            pos += 16;                                                        \
        }                                                                     \
        if (pos + 8 <= len)                                                   \
        {                                                                     \
            COPY_8BYTES(buf_out, (ptr + pos));                                \
            pos += 8;                                                         \
        }                                                                     \
        if (pos + 4 <= len)                                                   \
        {                                                                     \
            COPY_4BYTES(buf_out, (ptr + pos));                                \
            pos += 4;                                                         \
        }                                                                     \
        if (pos + 2 <= len)                                                   \
        {                                                                     \
            COPY_2BYTES(buf_out, (ptr + pos));                                \
            pos += 2;                                                         \
        }                                                                     \
        if (pos + 1 <= len)                                                   \
        {                                                                     \
            COPY_BYTE(buf_out, (ptr + pos));                                  \
            pos += 1;                                                         \
        }                                                                     \
        buf_out += pos;                                                       \
    }

#define _07_XX_ENCODE_STI_VL_COMMON_GUARDM(buf_out, data, data_len, field, n, m) \
    ENCODE_STI_VL_COMMON_GUARDM(buf_out, data, data_len, field, n, m)

#define _0F_09_ENCODE_MEMOS_SINGLE_GUARDM(buf_out, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len, n, m)                                           \
    {                                                                                                                                                              \
        ENCODE_FIELDS(buf_out, ARRAY, MEMOS); /*Arr Start*/                                         /* uint32  | size   1 */                                       \
        ENCODE_FIELDS(buf_out, OBJECT, MEMO); /*Obj start*/                                         /* uint32  | size   1 */                                       \
        _07_XX_ENCODE_STI_VL_COMMON_GUARDM(buf_out, type_ptr, type_len, MEMO_TYPE, n, m);           /* STI_VL  | size   type_len + (type_len <= 192 ? 2 : 3)*/     \
        _07_XX_ENCODE_STI_VL_COMMON_GUARDM(buf_out, data_ptr, data_len, MEMO_DATA, n, m + 1);       /* STI_VL  | size   data_len + (data_len <= 192 ? 2 : 3)*/     \
        _07_XX_ENCODE_STI_VL_COMMON_GUARDM(buf_out, format_ptr, format_len, MEMO_FORMAT, n, m + 2); /* STI_VL  | size   format_len + (format_len <= 192 ? 2 : 3)*/ \
        ENCODE_FIELDS(buf_out, OBJECT, END); /*Obj end*/                                            /* uint32  | size   1 */                                       \
        ENCODE_FIELDS(buf_out, ARRAY, END); /*Arr End*/                                             /* uint32  | size   1 */                                       \
    }

#define _0F_09_ENCODE_MEMOS_DUO_GUARDM(buf_out, type1_ptr, type1_len, format1_ptr, format1_len, data1_ptr, data1_len, type2_ptr, type2_len, format2_ptr, format2_len, data2_ptr, data2_len, n, m) \
    {                                                                                                                                                                                             \
        ENCODE_FIELDS(buf_out, ARRAY, MEMOS); /*Arr Start*/                                           /* uint32  | size   1 */                                                                    \
        ENCODE_FIELDS(buf_out, OBJECT, MEMO); /*Obj start*/                                           /* uint32  | size   1 */                                                                    \
        _07_XX_ENCODE_STI_VL_COMMON_GUARDM(buf_out, type1_ptr, type1_len, MEMO_TYPE, n, m);           /* STI_VL  | size   type_len + (type_len <= 192 ? 2 : 3)*/                                  \
        _07_XX_ENCODE_STI_VL_COMMON_GUARDM(buf_out, data1_ptr, data1_len, MEMO_DATA, n, m + 1);       /* STI_VL  | size   data_len + (data_len <= 192 ? 2 : 3)*/                                  \
        _07_XX_ENCODE_STI_VL_COMMON_GUARDM(buf_out, format1_ptr, format1_len, MEMO_FORMAT, n, m + 2); /* STI_VL  | size   format_len + (format_len <= 192 ? 2 : 3)*/                              \
        ENCODE_FIELDS(buf_out, OBJECT, END); /*Obj end*/                                              /* uint32  | size   1 */                                                                    \
        ENCODE_FIELDS(buf_out, OBJECT, MEMO); /*Obj start*/                                           /* uint32  | size   1 */                                                                    \
        _07_XX_ENCODE_STI_VL_COMMON_GUARDM(buf_out, type2_ptr, type2_len, MEMO_TYPE, n, m + 3);       /* STI_VL  | size   type_len + (type_len <= 192 ? 2 : 3)*/                                  \
        _07_XX_ENCODE_STI_VL_COMMON_GUARDM(buf_out, data2_ptr, data2_len, MEMO_DATA, n, m + 4);       /* STI_VL  | size   data_len + (data_len <= 192 ? 2 : 3)*/                                  \
        _07_XX_ENCODE_STI_VL_COMMON_GUARDM(buf_out, format2_ptr, format2_len, MEMO_FORMAT, n, m + 5); /* STI_VL  | size   format_len + (format_len <= 192 ? 2 : 3)*/                              \
        ENCODE_FIELDS(buf_out, OBJECT, END); /*Obj end*/                                              /* uint32  | size   1 */                                                                    \
        ENCODE_FIELDS(buf_out, ARRAY, END); /*Arr End*/                                               /* uint32  | size   1 */                                                                    \
    }

/**************************************************************************/
/**********************Guarded macro.h duplicates**************************/
/**************************************************************************/

#define ENCODE_TL_GUARDM(buf_out, tlamt, amount_type, n, m) \
    {                                                       \
        uint8_t uat = amount_type;                          \
        buf_out[0] = 0x60U + (uat & 0x0FU);                 \
        COPY_40BYTES((buf_out + 1), tlamt);                 \
        COPY_8BYTES((buf_out + 41), (tlamt + 40));          \
        COPY_BYTE((buf_out + 49), (tlamt + 48));            \
        buf_out += ENCODE_TL_SIZE;                          \
    }

#define ENCODE_TL_AMOUNT_GUARDM(buf_out, drops, n, m) \
    ENCODE_TL_GUARDM(buf_out, drops, amAMOUNT, n, m);
#define _06_01_ENCODE_TL_AMOUNT_GUARDM(buf_out, drops, n, m) \
    ENCODE_TL_AMOUNT_GUARDM(buf_out, drops, n, m);

#define PREPARE_PAYMENT_SIMPLE_TRUSTLINE_GUARDM(buf_out_master, tlamt, to_address, dest_tag_raw, src_tag_raw, n, m) \
    {                                                                                                               \
        uint8_t *buf_out = buf_out_master;                                                                          \
        uint8_t acc[20];                                                                                            \
        uint32_t dest_tag = (dest_tag_raw);                                                                         \
        uint32_t src_tag = (src_tag_raw);                                                                           \
        uint32_t cls = (uint32_t)ledger_seq();                                                                      \
        hook_account(SBUF(acc));                                                                                    \
        _01_02_ENCODE_TT(buf_out, ttPAYMENT);                 /* uint16  | size   3 */                              \
        _02_02_ENCODE_FLAGS(buf_out, tfCANONICAL);            /* uint32  | size   5 */                              \
        _02_03_ENCODE_TAG_SRC(buf_out, src_tag);              /* uint32  | size   5 */                              \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                   /* uint32  | size   5 */                              \
        _02_14_ENCODE_TAG_DST(buf_out, dest_tag);             /* uint32  | size   5 */                              \
        _02_26_ENCODE_FLS(buf_out, cls + 1);                  /* uint32  | size   6 */                              \
        _02_27_ENCODE_LLS(buf_out, cls + 5);                  /* uint32  | size   6 */                              \
        _06_01_ENCODE_TL_AMOUNT_GUARDM(buf_out, tlamt, n, m); /* amount  | size  49 */                              \
        uint8_t *fee_ptr = buf_out;                                                                                 \
        _06_08_ENCODE_DROPS_FEE(buf_out, 0);                                    /* amount  | size   9 */            \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);                             /* pk      | size  35 */            \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);                                /* account | size  22 */            \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address);                         /* account | size  22 */            \
        etxn_details((uint32_t)buf_out, PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE); /* emitdet | size 1?? */            \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);                         \
        \ 
        CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee);                                                               \
    }

/**************************************************************************/
/*****************Macros to prepare transactions with memos****************/
/**************************************************************************/

/////////// Macros to prepare trustline payment with memos. ///////////

#define PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(type_len, format_len, data_len) \
    ((type_len + (type_len <= 192 ? 2 : 3) + format_len + (format_len <= 192 ? 2 : 3) + data_len + (data_len <= 192 ? 2 : 3)) + PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE + 4)
#define PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_M(buf_out_master, tlamt, drops_fee_raw, to_address, dest_tag_raw, src_tag_raw, type, type_len, format, format_len, data, data_len, m) \
    {                                                                                                                                                                                       \
        uint8_t *buf_out = buf_out_master;                                                                                                                                                  \
        uint8_t acc[20];                                                                                                                                                                    \
        uint32_t dest_tag = (dest_tag_raw);                                                                                                                                                 \
        uint32_t src_tag = (src_tag_raw);                                                                                                                                                   \
        uint32_t cls = (uint32_t)ledger_seq();                                                                                                                                              \
        hook_account(SBUF(acc));                                                                                                                                                            \
        _01_02_ENCODE_TT(buf_out, ttPAYMENT);                 /* uint16  | size   3 */                                                                                                      \
        _02_02_ENCODE_FLAGS(buf_out, tfCANONICAL);            /* uint32  | size   5 */                                                                                                      \
        _02_03_ENCODE_TAG_SRC(buf_out, src_tag);              /* uint32  | size   5 */                                                                                                      \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                   /* uint32  | size   5 */                                                                                                      \
        _02_14_ENCODE_TAG_DST(buf_out, dest_tag);             /* uint32  | size   5 */                                                                                                      \
        _02_26_ENCODE_FLS(buf_out, cls + 1);                  /* uint32  | size   6 */                                                                                                      \
        _02_27_ENCODE_LLS(buf_out, cls + 5);                  /* uint32  | size   6 */                                                                                                      \
        _06_01_ENCODE_TL_AMOUNT_GUARDM(buf_out, tlamt, 1, m); /* amount  | size  49 */                                                                                                      \
        uint8_t *fee_ptr = buf_out;                                                                                                                                                         \
        _06_08_ENCODE_DROPS_FEE(buf_out, 0);            /* amount  | size   9 */                                                                                                            \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);     /* pk      | size  35 */                                                                                                            \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);        /* account | size  22 */                                                                                                            \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address); /* account | size  22 */                                                                                                            \
        _0F_09_ENCODE_MEMOS_SINGLE_GUARDM(buf_out, type, type_len, format, format_len, data, data_len, 1, m);                                                                               \
        etxn_details((uint32_t)buf_out, PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(type_len, format_len, data_len)); /* emitdet | size 1?? */                                       \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(type_len, format_len, data_len));                                                    \
        \ 
        CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee);                                                                                                                                       \
    }

/////////// Macros to prepare simple payment with memos. ///////////
#define PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_SIZE(type_len, format_len, data_len) \
    ((type_len + (type_len <= 192 ? 2 : 3) + format_len + (format_len <= 192 ? 2 : 3) + data_len + (data_len <= 192 ? 2 : 3)) + PREPARE_PAYMENT_SIMPLE_SIZE + 4)
#define PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_M(buf_out_master, drops_amount_raw, to_address, dest_tag_raw, src_tag_raw, type, type_len, format, format_len, data, data_len, m) \
    {                                                                                                                                                                         \
        uint8_t *buf_out = buf_out_master;                                                                                                                                    \
        uint8_t acc[20];                                                                                                                                                      \
        uint64_t drops_amount = (drops_amount_raw);                                                                                                                           \
        uint32_t dest_tag = (dest_tag_raw);                                                                                                                                   \
        uint32_t src_tag = (src_tag_raw);                                                                                                                                     \
        uint32_t cls = (uint32_t)ledger_seq();                                                                                                                                \
        hook_account(SBUF(acc));                                                                                                                                              \
        _01_02_ENCODE_TT(buf_out, ttPAYMENT);              /* uint16  | size   3 */                                                                                           \
        _02_02_ENCODE_FLAGS(buf_out, tfCANONICAL);         /* uint32  | size   5 */                                                                                           \
        _02_03_ENCODE_TAG_SRC(buf_out, src_tag);           /* uint32  | size   5 */                                                                                           \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                /* uint32  | size   5 */                                                                                           \
        _02_14_ENCODE_TAG_DST(buf_out, dest_tag);          /* uint32  | size   5 */                                                                                           \
        _02_26_ENCODE_FLS(buf_out, cls + 1);               /* uint32  | size   6 */                                                                                           \
        _02_27_ENCODE_LLS(buf_out, cls + 5);               /* uint32  | size   6 */                                                                                           \
        _06_01_ENCODE_DROPS_AMOUNT(buf_out, drops_amount); /* amount  | size   9 */                                                                                           \
        uint8_t *fee_ptr = buf_out;                                                                                                                                           \
        _06_08_ENCODE_DROPS_FEE(buf_out, 0);            /* amount  | size   9 */                                                                                              \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);     /* pk      | size  35 */                                                                                              \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);        /* account | size  22 */                                                                                              \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address); /* account | size  22 */                                                                                              \
        _0F_09_ENCODE_MEMOS_SINGLE_GUARDM(buf_out, type, type_len, format, format_len, data, data_len, 1, m);                                                                 \
        etxn_details((uint32_t)buf_out, PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_SIZE(type_len, format_len, data_len)); /* emitdet | size 1?? */                                   \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_SIZE(type_len, format_len, data_len));                                                \
        \ 
        CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee);                                                                                                                         \
    }

/**************************************************************************/
/***********************NFT related transactions***************************/
/**************************************************************************/

/////////// Macro to prepare a simple transaction. ///////////

#ifdef HAS_CALLBACK
#define PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE 309
#else
#define PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE 287
#endif
#define PREPARE_PAYMENT_SIMPLE_TRUSTLINE(buf_out_master, tlamt, to_address, dest_tag_raw, src_tag_raw)   \
    {                                                                                                    \
        uint8_t *buf_out = buf_out_master;                                                               \
        uint8_t acc[20];                                                                                 \
        uint32_t dest_tag = (dest_tag_raw);                                                              \
        uint32_t src_tag = (src_tag_raw);                                                                \
        uint32_t cls = (uint32_t)ledger_seq();                                                           \
        hook_account(SBUF(acc));                                                                         \
        _01_02_ENCODE_TT(buf_out, ttPAYMENT);                 /* uint16  | size   3 */                   \
        _02_02_ENCODE_FLAGS(buf_out, tfCANONICAL);            /* uint32  | size   5 */                   \
        _02_03_ENCODE_TAG_SRC(buf_out, src_tag);              /* uint32  | size   5 */                   \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                   /* uint32  | size   5 */                   \
        _02_14_ENCODE_TAG_DST(buf_out, dest_tag);             /* uint32  | size   5 */                   \
        _02_26_ENCODE_FLS(buf_out, cls + 1);                  /* uint32  | size   6 */                   \
        _02_27_ENCODE_LLS(buf_out, cls + 5);                  /* uint32  | size   6 */                   \
        _06_01_ENCODE_TL_AMOUNT_GUARDM(buf_out, tlamt, 1, 1); /* amount  | size  48 */                   \
        uint8_t *fee_ptr = buf_out;                                                                      \
        _06_08_ENCODE_DROPS_FEE(buf_out, 0);                                    /* amount  | size   9 */ \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);                             /* pk      | size  35 */ \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);                                /* account | size  22 */ \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address);                         /* account | size  22 */ \
        etxn_details((uint32_t)buf_out, PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE); /* emitdet | size 1?? */ \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);              \
        \ 
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                                           \
    }

/////////// Macro to prepare a nft mint. ///////////

#define PREPARE_NFT_MINT_SIZE(uri_len) \
    (uri_len + 240)
#define PREPARE_NFT_MINT(buf_out_master, tflag, transfer_fee, taxon, uri, uri_len)    \
    {                                                                                 \
        uint8_t *buf_out = buf_out_master;                                            \
        uint8_t acc[20];                                                              \
        uint32_t cls = (uint32_t)ledger_seq();                                        \
        hook_account(SBUF(acc));                                                      \
        _01_02_ENCODE_TT(buf_out, ttNFT_MINT);   /* uint16  | size   3 */             \
        _01_04_ENCODE_TF(buf_out, transfer_fee); /* uint16  | size   3 */             \
        _02_02_ENCODE_FLAGS(buf_out, tflag);     /* uint32  | size   5 */             \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);      /* uint32  | size   5 */             \
        _02_26_ENCODE_FLS(buf_out, cls + 1);     /* uint32  | size   6 */             \
        _02_27_ENCODE_LLS(buf_out, cls + 5);     /* uint32  | size   6 */             \
        _02_42_ENCODE_TXON(buf_out, taxon);      /* uint32  | size   6 */             \
        uint8_t *fee_ptr = buf_out;                                                   \
        _06_08_ENCODE_DROPS_FEE(buf_out, 0);        /* amount  | size   9 */          \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out); /* pk      | size  35 */          \
        _07_05_ENCODE_URI(buf_out, uri, uri_len);   /* account | size  uri_len + 2 */ \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);    /* account | size  22 */          \
        etxn_details((uint32_t)buf_out, 138);       /* emitdet | size 138 */          \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_NFT_MINT_SIZE(uri_len));  \
        \ 
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                        \
    }

/////////// Macro to prepare a nft burn. ///////////

#define PREPARE_NFT_BURN_SIZE 284
#define PREPARE_NFT_BURN(buf_out_master, tknid, owner)                             \
    {                                                                              \
        uint8_t *buf_out = buf_out_master;                                         \
        uint8_t acc[20];                                                           \
        uint32_t cls = (uint32_t)ledger_seq();                                     \
        hook_account(SBUF(acc));                                                   \
        _01_02_ENCODE_TT(buf_out, ttNFT_BURN);            /* uint16  | size   3 */ \
        _02_02_ENCODE_FLAGS(buf_out, 0x00000000);         /* uint32  | size   5 */ \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);               /* uint32  | size   5 */ \
        _02_26_ENCODE_FLS(buf_out, cls + 1);              /* uint32  | size   6 */ \
        _02_27_ENCODE_LLS(buf_out, cls + 5);              /* uint32  | size   6 */ \
        _05_10_ENCODE_EMIT_PARENT_TXN_ID(buf_out, tknid); /* tknid   | size  33 */ \
        uint8_t *fee_ptr = buf_out;                                                \
        _06_08_ENCODE_DROPS_FEE(buf_out, 0);         /* amount  | size   9 */      \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);  /* pk      | size  35 */      \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);     /* account | size  22 */      \
        _08_02_ENCODE_ACCOUNT_OWNER(buf_out, owner); /* account | size  22 */      \
        etxn_details((uint32_t)buf_out, 138);        /* emitdet | size 138 */      \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_NFT_BURN_SIZE);        \
        \ 
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                     \
    }

/////////// Macro to prepare a nft sell offer. ///////////

#define PREPARE_NFT_SELL_OFFER_SIZE 298
#define PREPARE_NFT_SELL_OFFER(buf_out_master, drops_amount_raw, to_address, tknid) \
    {                                                                               \
        uint8_t *buf_out = buf_out_master;                                          \
        uint8_t acc[20];                                                            \
        uint64_t drops_amount = (drops_amount_raw);                                 \
        uint32_t cls = (uint32_t)ledger_seq();                                      \
        hook_account(SBUF(acc));                                                    \
        _01_02_ENCODE_TT(buf_out, ttNFT_OFFER);            /* uint16  | size   3 */ \
        _02_02_ENCODE_FLAGS(buf_out, tfSellToken);         /* uint32  | size   5 */ \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                /* uint32  | size   5 */ \
        _02_10_ENCODE_EXPIRATION_MAX(buf_out);             /* uint32  | size   5 */ \
        _02_26_ENCODE_FLS(buf_out, cls + 1);               /* uint32  | size   6 */ \
        _02_27_ENCODE_LLS(buf_out, cls + 5);               /* uint32  | size   6 */ \
        _06_01_ENCODE_DROPS_AMOUNT(buf_out, drops_amount); /* amount  | size   9 */ \
        _05_10_ENCODE_EMIT_PARENT_TXN_ID(buf_out, tknid);  /* tknid   | size  33 */ \
        uint8_t *fee_ptr = buf_out;                                                 \
        _06_08_ENCODE_DROPS_FEE(buf_out, 0);            /* amount  | size   9 */    \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);     /* pk      | size  35 */    \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);        /* account | size  22 */    \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address); /* account | size  22 */    \
        etxn_details((uint32_t)buf_out, 138);           /* emitdet | size 138 */    \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_NFT_SELL_OFFER_SIZE);   \
        \ 
        CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee);                               \
    }

/////////// Macro to prepare a nft buy offer. ///////////

#define PREPARE_NFT_BUY_OFFER_SIZE 298
#define PREPARE_NFT_BUY_OFFER(buf_out_master, drops_amount_raw, to_address, tknid)  \
    {                                                                               \
        uint8_t *buf_out = buf_out_master;                                          \
        uint8_t acc[20];                                                            \
        uint64_t drops_amount = (drops_amount_raw);                                 \
        uint32_t cls = (uint32_t)ledger_seq();                                      \
        hook_account(SBUF(acc));                                                    \
        _01_02_ENCODE_TT(buf_out, ttNFT_OFFER);            /* uint16  | size   3 */ \
        _02_02_ENCODE_FLAGS(buf_out, tfBuyToken);          /* uint32  | size   5 */ \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                /* uint32  | size   5 */ \
        _02_10_ENCODE_EXPIRATION_MAX(buf_out);             /* uint32  | size   5 */ \
        _02_26_ENCODE_FLS(buf_out, cls + 1);               /* uint32  | size   6 */ \
        _02_27_ENCODE_LLS(buf_out, cls + 5);               /* uint32  | size   6 */ \
        _06_01_ENCODE_DROPS_AMOUNT(buf_out, drops_amount); /* amount  | size   9 */ \
        _05_10_ENCODE_EMIT_PARENT_TXN_ID(buf_out, tknid);  /* tknid   | size  33 */ \
        uint8_t *fee_ptr = buf_out;                                                 \
        _06_08_ENCODE_DROPS_FEE(buf_out, 0);              /* amount  | size   9 */  \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);       /* pk      | size  35 */  \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);          /* account | size  22 */  \
        _08_02_ENCODE_ACCOUNT_OWNER(buf_out, to_address); /* account | size  22 */  \
        etxn_details((uint32_t)buf_out, 138);             /* emitdet | size 138 */  \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_NFT_BUY_OFFER_SIZE);    \
        \ 
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                      \
    }

/////////// Macro to prepare a nft buy offer.(IOU) ///////////

#define PREPARE_NFT_BUY_OFFER_TRUSTLINE_SIZE 338
#define PREPARE_NFT_BUY_OFFER_TRUSTLINE_SIZE 338
#define PREPARE_NFT_BUY_OFFER_TRUSTLINE(buf_out_master, tlamt, to_address, tknid)          \
    {                                                                                      \
        uint8_t *buf_out = buf_out_master;                                                 \
        uint8_t acc[20];                                                                   \
        uint32_t cls = (uint32_t)ledger_seq();                                             \
        hook_account(SBUF(acc));                                                           \
        _01_02_ENCODE_TT(buf_out, ttNFT_OFFER);               /* uint16  | size   3 */     \
        _02_02_ENCODE_FLAGS(buf_out, tfBuyToken);             /* uint32  | size   5 */     \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                   /* uint32  | size   5 */     \
        _02_10_ENCODE_EXPIRATION_MAX(buf_out);                /* uint32  | size   5 */     \
        _02_26_ENCODE_FLS(buf_out, cls + 1);                  /* uint32  | size   6 */     \
        _02_27_ENCODE_LLS(buf_out, cls + 5);                  /* uint32  | size   6 */     \
        _06_01_ENCODE_TL_AMOUNT_GUARDM(buf_out, tlamt, 1, 1); /* amount  | size  49 */     \
        _05_10_ENCODE_EMIT_PARENT_TXN_ID(buf_out, tknid);     /* tknid   | size  33 */     \
        uint8_t *fee_ptr = buf_out;                                                        \
        _06_08_ENCODE_DROPS_FEE(buf_out, 0);              /* amount  | size   9 */         \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);       /* pk      | size  35 */         \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);          /* account | size  22 */         \
        _08_02_ENCODE_ACCOUNT_OWNER(buf_out, to_address); /* account | size  22 */         \
        etxn_details((uint32_t)buf_out, 138);             /* emitdet | size 138 */         \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_NFT_BUY_OFFER_TRUSTLINE_SIZE); \
        \ 
        CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee);                                      \
    }

/**************************************************************************/
/************** Set Hook Related Transactions *****************************/
/**************************************************************************/
#define OP_HOOK_INSTALLATION 0
#define OP_HOOK_DELETION 1
#define ENCODE_HOOK_INSTALLATION_SIZE 75
#define ENCODE_HOOK_DELETION_SIZE 9
#define ENCODE_HOOK_NO_OPERATION_SIZE 2
#define GET_HOOKSET_OPERATION_SIZE(operation_type)                                                                                           \
    (operation_type == OP_HOOK_INSTALLATION ? ENCODE_HOOK_INSTALLATION_SIZE : operation_type == OP_HOOK_DELETION ? ENCODE_HOOK_DELETION_SIZE \
                                                                                                                 : ENCODE_HOOK_NO_OPERATION_SIZE)

#define PREPARE_SET_HOOK_TRANSACTION_SIZE(operation_order) \
    (231 + GET_HOOKSET_OPERATION_SIZE(operation_order[0]) + GET_HOOKSET_OPERATION_SIZE(operation_order[1]) + GET_HOOKSET_OPERATION_SIZE(operation_order[2]) + GET_HOOKSET_OPERATION_SIZE(operation_order[3]))

#define ENCODE_HOOK_OBJECT(buf_out, hash, namespace, operation)                       \
    {                                                                                 \
        ENCODE_FIELDS(buf_out, OBJECT, HOOK); /*Obj start*/ /* uint32  | size   1 */  \
        if (operation == OP_HOOK_INSTALLATION || operation == OP_HOOK_DELETION)       \
        {                                                                             \
            _02_02_ENCODE_FLAGS(buf_out, tfHookOveride); /* uint32  | size   5 */     \
            if (operation == OP_HOOK_INSTALLATION)                                    \
            {                                                                         \
                _05_31_ENCODE_HOOKHASH(buf_out, hash);       /* uint256 | size  34 */ \
                _05_32_ENCODE_NAMESPACE(buf_out, namespace); /* uint256 | size  34 */ \
            }                                                                         \
            else                                                                      \
            {                                                                         \
                _07_11_ENCODE_DELETEHOOK(buf_out); /* blob    | size   2 */           \
            }                                                                         \
        }                                                                             \
        ENCODE_FIELDS(buf_out, OBJECT, END); /*Arr End*/ /* uint32  | size   1 */     \
    }

#define PREPARE_SET_HOOK_TRANSACTION(buf_out_master, operation_order, first_hash_pointer, namespace)                  \
    {                                                                                                                 \
        uint8_t *buf_out = buf_out_master;                                                                            \
        uint32_t cls = (uint32_t)ledger_seq();                                                                        \
        uint8_t acc[20];                                                                                              \
        hook_account(SBUF(acc));                                                                                      \
        _01_02_ENCODE_TT(buf_out, ttHOOK_SET);   /* uint16  | size   3 */                                             \
        _02_02_ENCODE_FLAGS(buf_out, tfOnlyXRP); /* uint32  | size   5 */                                             \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);      /* uint32  | size   5 */                                             \
        _02_26_ENCODE_FLS(buf_out, cls + 1);     /* uint32  | size   6 */                                             \
        _02_27_ENCODE_LLS(buf_out, cls + 5);     /* uint32  | size   6 */                                             \
        uint8_t *fee_ptr = buf_out;                                                                                   \
        _06_08_ENCODE_DROPS_FEE(buf_out, 0);                /* amount  | size   9 */                                  \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);         /* pk      | size  35 */                                  \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);            /* account | size  22 */                                  \
        ENCODE_FIELDS(buf_out, ARRAY, HOOKS); /*Arr Start*/ /* uint32  | size   1 */                                  \
        ENCODE_HOOK_OBJECT(buf_out, first_hash_pointer, namespace, operation_order[0]);                               \
        ENCODE_HOOK_OBJECT(buf_out, (first_hash_pointer + HASH_SIZE), namespace, operation_order[1]);                 \
        ENCODE_HOOK_OBJECT(buf_out, (first_hash_pointer + (2 * HASH_SIZE)), namespace, operation_order[2]);           \
        ENCODE_HOOK_OBJECT(buf_out, (first_hash_pointer + (3 * HASH_SIZE)), namespace, operation_order[3]);           \
        ENCODE_FIELDS(buf_out, ARRAY, END); /*Arr End*/                                      /* uint32  | size   1 */ \
        etxn_details((uint32_t)buf_out, PREPARE_SET_HOOK_TRANSACTION_SIZE(operation_order)); /* emitdet | size 138 */ \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_SET_HOOK_TRANSACTION_SIZE(operation_order));              \
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                                                        \
    }

/**************************************************************************/
/******************Macros with evernode specific logic*********************/
/**************************************************************************/

#define PREPARE_PAYMENT_PRUNED_HOST_REBATE_SIZE \
    (PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(strlen(DEAD_HOST_PRUNE_REF), strlen(FORMAT_TEXT), strlen(PRUNE_MESSAGE)))
#define PREPARE_PAYMENT_PRUNED_HOST_REBATE(buf_out_master, tlamt, drops_fee_raw, to_address)                                                                                                                                                  \
    {                                                                                                                                                                                                                                         \
        PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_M(buf_out_master, tlamt, drops_fee_raw, to_address, 0, 0, DEAD_HOST_PRUNE_REF, strlen(DEAD_HOST_PRUNE_REF), FORMAT_TEXT, strlen(FORMAT_TEXT), PRUNE_MESSAGE, strlen(PRUNE_MESSAGE), 1); \
    }

#define PREPARE_MIN_PAYMENT_PRUNED_HOST_SIZE \
    (PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_SIZE(strlen(DEAD_HOST_PRUNE_REF), strlen(FORMAT_TEXT), strlen(PRUNE_MESSAGE)))
#define PREPARE_MIN_PAYMENT_PRUNED_HOST(buf_out_master, drops_fee_raw, to_address)                                                                                                                                           \
    {                                                                                                                                                                                                                        \
        PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_M(buf_out_master, drops_fee_raw, to_address, 0, 0, DEAD_HOST_PRUNE_REF, strlen(DEAD_HOST_PRUNE_REF), FORMAT_TEXT, strlen(FORMAT_TEXT), PRUNE_MESSAGE, strlen(PRUNE_MESSAGE), 1); \
    }

#define PREPARE_PAYMENT_HOST_REWARD_SIZE \
    (PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(13, 0, 0))
#define PREPARE_PAYMENT_HOST_REWARD(buf_out_master, tlamt, drops_fee_raw, to_address)                                                                            \
    {                                                                                                                                                            \
        PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_M(buf_out_master, tlamt, drops_fee_raw, to_address, 0, 0, HOST_REWARD, 13, empty_ptr, 0, empty_ptr, 0, 1); \
    }

#endif