#ifndef EVERNODE_INCLUDED
#define EVERNODE_INCLUDED 1

#include "constants.h"

/****** Field and Type codes ******/
#define ARRAY 0xF0U
#define OBJECT 0xE0U
#define MEMOS 0X9
#define MEMO 0XA
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
#define IS_EVR(is_evr, amount_buffer, issuer_accid)    \
    is_evr = 1;                                        \
    for (int i = 0; GUARD(20), i < 20; ++i)            \
    {                                                  \
        if (amount_buffer[i + 8] != evr_currency[i] || \
            amount_buffer[i + 28] != issuer_accid[i])  \
        {                                              \
            is_evr = 0;                                \
            break;                                     \
        }                                              \
    }

#define ASCII_TO_HEX(val)                                                              \
    val >= 'A' && val <= 'F' ? (val - 'A') + 10 : val >= '0' && val <= '9' ? val - '0' \
                                                                           : -1;

#define HEXSTR_TO_BYTES(byte_ptr, hexstr_ptr, hexstr_len)          \
    {                                                              \
        for (int i = 0; GUARD(hexstr_len), i < hexstr_len; i += 2) \
        {                                                          \
            int val1 = (int)hexstr_ptr[i];                         \
            int val2 = (int)hexstr_ptr[i + 1];                     \
            val1 = ASCII_TO_HEX(val1);                             \
            val2 = ASCII_TO_HEX(val2);                             \
            byte_ptr[i / 2] = ((val1 * 16) + val2);                \
        }                                                          \
    }

#define STR_TO_UINT(number, str_ptr, str_len)                      \
    {                                                              \
        number = 0;                                                \
        for (int i = 0; GUARD(MAX_UINT_STR_LEN), i < str_len; i++) \
            number = number * 10 + (int)str_ptr[i] - '0';          \
    }

#define IS_BUF_EMPTY_GUARD(is_empty, buf, buflen, n)    \
    is_empty = 1;                                       \
    for (int i = 0; GUARD(buflen * n), i < buflen; ++i) \
    {                                                   \
        if (buf[i] != 0)                                \
        {                                               \
            is_empty = 0;                               \
            break;                                      \
        }                                               \
    }

#define IS_BUF_EMPTY(is_empty, buf, buflen) \
    IS_BUF_EMPTY_GUARD(is_empty, buf, buflen, 1)

#define MAX(num1, num2) \
    ((num1 > num2) ? num1 : num2)

#define MIN(num1, num2) \
    ((num1 > num2) ? num2 : num1)

#define CEIL(dividend, divisor) \
    ((dividend / divisor) + ((dividend % divisor) != 0))

// It gives -29 (EXPONENT_UNDERSIZED) when balance is zero. Need to read and experiment more.
#define IS_FLOAT_ZERO(float) \
    (float == 0 || float == -29)

// Provide m >= 1 to indicate in which code line macro will hit.
// Provide n >= 1 to indicate how many times the macro will be hit on the line of code.
// e.g. if it is in a loop that loops 10 times n = 10
// If it is used 3 times inside a macro use m = 1,2,3
#define COPY_BUF_GUARDM(lhsbuf, lhsbuf_spos, rhsbuf, rhsbuf_spos, len, n, m) \
    {                                                                        \
        for (int i = 0; GUARDM((n * (len + 1)), m), i < len; ++i)            \
            lhsbuf[lhsbuf_spos + i] = rhsbuf[rhsbuf_spos + i];               \
    }

#define COPY_BUF_GUARD(lhsbuf, lhsbuf_spos, rhsbuf, rhsbuf_spos, len, n) \
    COPY_BUF_GUARDM(lhsbuf, lhsbuf_spos, rhsbuf, rhsbuf_spos, len, n, 1);

#define COPY_BUF(lhsbuf, lhsbuf_spos, rhsbuf, rhsbuf_spos, len) \
    COPY_BUF_GUARDM(lhsbuf, lhsbuf_spos, rhsbuf, rhsbuf_spos, len, 1, 1);

#define CLEAR_BUF(buf, buf_spos, len)             \
    {                                             \
        for (int i = 0; GUARD(len), i < len; ++i) \
            buf[buf_spos + i] = 0;                \
    }

#define COPY_BUF_NON_CONST_LEN_GUARDM(lhsbuf, lhsbuf_spos, rhsbuf, rhsbuf_spos, len, max_len, n, m) \
    {                                                                                               \
        for (int i = 0; GUARDM((n * (max_len + 1)), m), i < len; ++i)                               \
            lhsbuf[lhsbuf_spos + i] = rhsbuf[rhsbuf_spos + i];                                      \
    }

#define CLEAR_BUF_NON_CONST_LEN(buf, buf_spos, len, max_len) \
    {                                                        \
        for (int i = 0; GUARD(max_len), i < len; ++i)        \
            buf[buf_spos + i] = 0;                           \
    }

// when using this macro buf1len may be dynamic but buf2len must be static
// provide n >= 1 to indicate how many times the macro will be hit on the line of code
// e.g. if it is in a loop that loops 10 times n = 10

#define BUFFER_EQUAL_GUARDM(output, buf1, buf1len, buf2, buf2len, n, m)      \
    {                                                                        \
        output = ((buf1len) == (buf2len) ? 1 : 0);                           \
        for (int x = 0; GUARDM((buf2len) * (n), m), output && x < (buf2len); \
             ++x)                                                            \
            output = (buf1)[x] == (buf2)[x];                                 \
    }

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
        for (int i = 0; GUARDM(21 * n, m), i < 20; ++i)                           \
        {                                                                         \
            amt_out[i + 28] = issuer[i];                                          \
            amt_out[i + 8] = currency[i];                                         \
        }                                                                         \
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

#define BYTES_TO_HEXSTRM(hexstr_ptr, byte_ptr, byte_len, n)                                               \
    {                                                                                                     \
        char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'}; \
        for (int i = 0; GUARDM(byte_len, n), i < byte_len; i++)                                           \
        {                                                                                                 \
            hexstr_ptr[2 * i] = hexmap[(byte_ptr[i] & 0xF0) >> 4];                                        \
            hexstr_ptr[2 * i + 1] = hexmap[byte_ptr[i] & 0x0F];                                           \
        }                                                                                                 \
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
        UINT16_TO_BUF(token_id + 2, transaction_fee);                                             \
        COPY_BUF_GUARD(token_id, 4, accid, 0, 20, n);                                             \
        UINT32_TO_BUF(token_id + 24, taxon ^ ((NFT_TAXON_M * token_seq) + NFT_TAXON_C));          \
        UINT32_TO_BUF(token_id + 28, token_seq);                                                  \
    }

#define GENERATE_NFT_TOKEN_ID(token_id, tflag, transaction_fee, accid, taxon, token_seq) \
    GENERATE_NFT_TOKEN_ID_GUARD(token_id, tflag, transaction_fee, accid, taxon, token_seq, 1)

enum LedgerEntryType
{
    ltNFTOKEN_PAGE = 0x0050
};

const uint8_t page_mask[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

#define GET_NFT(account, nft_id, nft_exists, nft_issuer, nft_uri, nft_uri_len, nft_taxon, nft_flags, nft_tffee, nft_seq) \
    {                                                                                                                    \
        nft_exists = 0;                                                                                                  \
        uint8_t lo_keylet[34];                                                                                           \
        uint8_t buf[32] = {0};                                                                                           \
        COPY_BUF_GUARDM(buf, 0, account, 0, ACCOUNT_ID_SIZE, 1, 1);                                                      \
        lo_keylet[0] = (ltNFTOKEN_PAGE >> 8) & 0xFFU;                                                                    \
        lo_keylet[1] = (ltNFTOKEN_PAGE >> 0) & 0xFFU;                                                                    \
        COPY_BUF_GUARDM(lo_keylet, 2, buf, 0, 32, 1, 2);                                                                 \
                                                                                                                         \
        uint8_t id_keylet[34] = {0};                                                                                     \
        id_keylet[0] = (ltNFTOKEN_PAGE >> 8) & 0xFFU;                                                                    \
        id_keylet[1] = (ltNFTOKEN_PAGE >> 0) & 0xFFU;                                                                    \
        for (int i = 0; GUARDM(32, 3), i < 32; ++i)                                                                      \
            id_keylet[2 + i] = (lo_keylet[2 + i] & ~page_mask[i]) + (nft_id[i] & page_mask[i]);                          \
                                                                                                                         \
        uint8_t hi_keylet[34];                                                                                           \
        uint8_t id[32];                                                                                                  \
        COPY_BUF_GUARDM(id, 0, account, 0, ACCOUNT_ID_SIZE, 1, 4);                                                       \
        COPY_BUF_GUARDM(id, ACCOUNT_ID_SIZE, page_mask, ACCOUNT_ID_SIZE, 34, 1, 5);                                      \
        hi_keylet[0] = (ltNFTOKEN_PAGE >> 8) & 0xFFU;                                                                    \
        hi_keylet[1] = (ltNFTOKEN_PAGE >> 0) & 0xFFU;                                                                    \
        COPY_BUF_GUARDM(hi_keylet, 2, id, 0, 32, 1, 6);                                                                  \
                                                                                                                         \
        uint8_t nft_keylet[34];                                                                                          \
        if (ledger_keylet(SBUF(nft_keylet), SBUF(id_keylet), SBUF(hi_keylet)) != 34)                                     \
            rollback(SBUF("Evernode: Could not generate the ledger nft keylet."), 10);                                   \
                                                                                                                         \
        int64_t nfts_slot = slot_set(SBUF(nft_keylet), 0);                                                               \
        if (nfts_slot < 0)                                                                                               \
            rollback(SBUF("Evernode: Could not set ledger nft keylet in slot"), 10);                                     \
                                                                                                                         \
        nfts_slot = slot_subfield(nfts_slot, sfNFTokens, 0);                                                             \
        if (nfts_slot < 0)                                                                                               \
            rollback(SBUF("Evernode: Could not find sfNFTokens on ledger nft keylet"), 1);                               \
                                                                                                                         \
        uint8_t cur_id[NFT_TOKEN_ID_SIZE] = {0};                                                                         \
        uint8_t uri_read_buf[258];                                                                                       \
        int64_t uri_read_len;                                                                                            \
        for (int i = 0; GUARDM(32, 7), i < 32; ++i)                                                                      \
        {                                                                                                                \
            int64_t nft_slot = slot_subarray(nfts_slot, i, 0);                                                           \
            if (nft_slot >= 0)                                                                                           \
            {                                                                                                            \
                int64_t id_slot = slot_subfield(nft_slot, sfNFTokenID, 0);                                               \
                if (id_slot >= 0 && slot(SBUF(cur_id), id_slot) == NFT_TOKEN_ID_SIZE)                                    \
                {                                                                                                        \
                    int equal = 0;                                                                                       \
                    BUFFER_EQUAL_GUARDM(equal, cur_id, NFT_TOKEN_ID_SIZE, nft_id, NFT_TOKEN_ID_SIZE, 32, 8);             \
                    if (equal)                                                                                           \
                    {                                                                                                    \
                        int64_t uri_slot = slot_subfield(nft_slot, sfURI, 0);                                            \
                        uri_read_len = slot(SBUF(uri_read_buf), uri_slot);                                               \
                        nft_exists = 1;                                                                                  \
                        break;                                                                                           \
                    }                                                                                                    \
                }                                                                                                        \
            }                                                                                                            \
        }                                                                                                                \
        if (nft_exists)                                                                                                  \
        {                                                                                                                \
            if (uri_read_len >= 195)                                                                                     \
            {                                                                                                            \
                nft_uri_len = 193 + ((uri_read_buf[0] - 193) * 256) + uri_read_buf[1];                                   \
                COPY_BUF_NON_CONST_LEN_GUARDM(nft_uri, 0, uri_read_buf, 2, nft_uri_len, sizeof(uri_read_buf), 1, 9);     \
            }                                                                                                            \
            else                                                                                                         \
            {                                                                                                            \
                nft_uri_len = uri_read_buf[0];                                                                           \
                COPY_BUF_NON_CONST_LEN_GUARDM(nft_uri, 0, uri_read_buf, 1, nft_uri_len, sizeof(uri_read_buf), 1, 9);     \
            }                                                                                                            \
            nft_flags = UINT16_FROM_BUF(cur_id);                                                                         \
            nft_tffee = UINT16_FROM_BUF(cur_id + 2);                                                                     \
            COPY_BUF_GUARDM(nft_issuer, 0, cur_id, 4, ACCOUNT_ID_SIZE, 1, 10);                                           \
            uint32_t taxon = UINT32_FROM_BUF(cur_id + 24);                                                               \
            nft_seq = UINT32_FROM_BUF(cur_id + 28);                                                                      \
            nft_taxon = taxon ^ ((NFT_TAXON_M * nft_seq) + NFT_TAXON_C);                                                 \
        }                                                                                                                \
    }

#define POW_GUARD(x, y, output, n)               \
    {                                            \
        output = 1;                              \
        for (int it = 0; GUARD(n), it < y; ++it) \
            output *= x;                         \
    }

#define GET_EPOCH_REWARD_QUOTA(epoch, first_epoch_reward_quota, quota) \
    {                                                                  \
        uint32_t div;                                                  \
        POW_GUARD(2, epoch - 1, div, DEF_EPOCH_COUNT);                 \
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
        const uint32_t cur_moment = (cur_ledger_seq - moment_base_idx) / moment_size;                                                                                                             \
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

#define ENCODE_TXON_SIZE 6U
#define ENCODE_TXON(buf_out, tf) \
    ENCODE_UINT32_UNCOMMON(buf_out, tf, 0x2A);
#define _02_42_ENCODE_TXON(buf_out, tf) \
    ENCODE_TXON(buf_out, tf);

#define ENCODE_URI(buf_out, uri, uri_len)                                             \
    {                                                                                 \
        buf_out[0] = 0x75U;                                                           \
        buf_out[1] = uri_len;                                                         \
        for (int i = 0; GUARD(uri_len / 8 + (uri_len % 8 != 0)), i < uri_len; i += 8) \
            *(uint64_t *)(buf_out + i + 2) = *(uint64_t *)(uri + i);                  \
        buf_out += uri_len + 2;                                                       \
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

#define ENCODE_STI_VL_COMMON_GUARDM(buf_out, data, data_len, field, n, m)                         \
    {                                                                                             \
        uint8_t *ptr = (uint8_t *)&data;                                                          \
        uint8_t uf = field;                                                                       \
        buf_out[0] = 0x70U + (uf & 0x0FU);                                                        \
        if (data_len <= 192) /*Data legth is represented with 2 bytes if (>192)*/                 \
        {                                                                                         \
            buf_out[1] = data_len;                                                                \
            COPY_BUF_NON_CONST_LEN_GUARDM(buf_out, 2, ptr, 0, data_len, MAX_MEMO_DATA_LEN, n, m); \
            buf_out += (2 + data_len);                                                            \
        }                                                                                         \
        else                                                                                      \
        {                                                                                         \
            buf_out[0] = 0x70U + (uf & 0x0FU);                                                    \
            buf_out[1] = ((data_len - 193) / 256) + 193;                                          \
            buf_out[2] = data_len - (((buf_out[1] - 193) * 256) + 193);                           \
            COPY_BUF_NON_CONST_LEN_GUARDM(buf_out, 3, ptr, 0, data_len, MAX_MEMO_DATA_LEN, n, m); \
            buf_out += (3 + data_len);                                                            \
        }                                                                                         \
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
        for (int i = 1; GUARDM(49 * n, m), i < 49; ++i)     \
            buf_out[i] = tlamt[i - 1];                      \
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
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                                                      \
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
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                                                                                                                              \
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
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                                                                                                                \
    }

/**************************************************************************/
/***********************NFT related transactions***************************/
/**************************************************************************/

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
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                      \
    }

/////////// Macro to prepare a nft buy offer. ///////////

#define PREPARE_NFT_BUY_OFFER_SIZE 338
#define PREPARE_NFT_BUY_OFFER(buf_out_master, tlamt, to_address, tknid)            \
    {                                                                              \
        uint8_t *buf_out = buf_out_master;                                         \
        uint8_t acc[20];                                                           \
        uint32_t cls = (uint32_t)ledger_seq();                                     \
        hook_account(SBUF(acc));                                                   \
        _01_02_ENCODE_TT(buf_out, ttNFT_OFFER);           /* uint16  | size   3 */ \
        _02_02_ENCODE_FLAGS(buf_out, tfBuyToken);         /* uint32  | size   5 */ \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);               /* uint32  | size   5 */ \
        _02_10_ENCODE_EXPIRATION_MAX(buf_out);            /* uint32  | size   5 */ \
        _02_26_ENCODE_FLS(buf_out, cls + 1);              /* uint32  | size   6 */ \
        _02_27_ENCODE_LLS(buf_out, cls + 5);              /* uint32  | size   6 */ \
        _06_01_ENCODE_TL_AMOUNT(buf_out, tlamt);          /* amount  | size  49 */ \
        _05_10_ENCODE_EMIT_PARENT_TXN_ID(buf_out, tknid); /* tknid   | size  33 */ \
        uint8_t *fee_ptr = buf_out;                                                \
        _06_08_ENCODE_DROPS_FEE(buf_out, 0);              /* amount  | size   9 */ \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);       /* pk      | size  35 */ \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);          /* account | size  22 */ \
        _08_02_ENCODE_ACCOUNT_OWNER(buf_out, to_address); /* account | size  22 */ \
        etxn_details((uint32_t)buf_out, 138);             /* emitdet | size 138 */ \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_NFT_BUY_OFFER_SIZE);   \
        \ 
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                     \
    }

/**************************************************************************/
/******************Macros with evernode specific logic*********************/
/**************************************************************************/

#define PREPARE_PAYMENT_FOUNDATION_RETURN_SIZE \
    (PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(19, 0, 0))
#define PREPARE_PAYMENT_FOUNDATION_RETURN(buf_out_master, tlamt, drops_fee_raw, to_address)                                                                               \
    {                                                                                                                                                                     \
        PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_M(buf_out_master, tlamt, drops_fee_raw, to_address, 0, 0, FOUNDATION_REFUND_50, 19, empty_ptr, 0, empty_ptr, 0, 1); \
    }

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

#define POW_GUARD(x, y, output, n)               \
    {                                            \
        output = 1;                              \
        for (int it = 0; GUARD(n), it < y; ++it) \
            output *= x;                         \
    }

#define GET_EPOCH_REWARD_QUOTA(epoch, first_epoch_reward_quota, quota) \
    {                                                                  \
        uint32_t div;                                                  \
        POW_GUARD(2, epoch - 1, div, DEF_EPOCH_COUNT);                 \
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
        const uint32_t cur_moment = (cur_ledger_seq - moment_base_idx) / moment_size;                                                                                                             \
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

#define PREPARE_PAYMENT_HOST_REWARD_SIZE \
    (PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(13, 0, 0))
#define PREPARE_PAYMENT_HOST_REWARD(buf_out_master, tlamt, drops_fee_raw, to_address)                                                                            \
    {                                                                                                                                                            \
        PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_M(buf_out_master, tlamt, drops_fee_raw, to_address, 0, 0, HOST_REWARD, 13, empty_ptr, 0, empty_ptr, 0, 1); \
    }

#endif