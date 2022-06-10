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

/////////// Utility Macros. ///////////

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

#define ASCII_TO_HEX(val)    \
    {                        \
        switch (val)         \
        {                    \
        case 'A':            \
            val = 10;        \
            break;           \
        case 'B':            \
            val = 11;        \
            break;           \
        case 'C':            \
            val = 12;        \
            break;           \
        case 'D':            \
            val = 13;        \
            break;           \
        case 'E':            \
            val = 14;        \
            break;           \
        case 'F':            \
            val = 15;        \
            break;           \
        default:             \
            val = val - '0'; \
            break;           \
        }                    \
    }

#define HEXSTR_TO_BYTES(byte_ptr, hexstr_ptr, hexstr_len)          \
    {                                                              \
        for (int i = 0; GUARD(hexstr_len), i < hexstr_len; i += 2) \
        {                                                          \
            int val1 = (int)hexstr_ptr[i];                         \
            int val2 = (int)hexstr_ptr[i + 1];                     \
            ASCII_TO_HEX(val1)                                     \
            ASCII_TO_HEX(val2)                                     \
            byte_ptr[i / 2] = ((val1 * 16) + val2);                \
        }                                                          \
    }

#define STR_TO_UINT(number, str_ptr, str_len)                      \
    {                                                              \
        number = 0;                                                \
        for (int i = 0; GUARD(MAX_UINT_STR_LEN), i < str_len; i++) \
            number = number * 10 + (int)str_ptr[i] - '0';          \
    }

#define IS_BUF_EMPTY_GUARD(is_empty, buf, n)                      \
    is_empty = 1;                                                 \
    for (int i = 0; GUARD(sizeof(buf) * n), i < sizeof(buf); ++i) \
    {                                                             \
        if (buf[i] != 0)                                          \
        {                                                         \
            is_empty = 0;                                         \
            break;                                                \
        }                                                         \
    }

#define IS_BUF_EMPTY(is_empty, buf) \
    IS_BUF_EMPTY_GUARD(is_empty, buf, 1)

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

/////////// Guarded hookmacro.h duplicates. ///////////

#define ENCODE_TL_GUARDM(buf_out, tlamt, amount_type, n, m) \
    {                                                       \
        uint8_t uat = amount_type;                          \
        buf_out[0] = 0x60U + (uat & 0x0FU);                 \
        for (int i = 1; GUARDM(49 * n, m), i < 49; ++i)     \
            buf_out[i] = tlamt[i - 1];                      \
        buf_out += ENCODE_TL_SIZE;                          \
    }

#define ENCODE_TL_SENDMAX_GUARDM(buf_out, drops, n, m) \
    ENCODE_TL_GUARDM(buf_out, drops, amSENDMAX, n, m);

#define _06_09_ENCODE_TL_SENDMAX_GUARDM(buf_out, drops, n, m) \
    ENCODE_TL_SENDMAX_GUARDM(buf_out, drops, n, n);

#define ENCODE_TL_AMOUNT_GUARDM(buf_out, drops, n, m) \
    ENCODE_TL_GUARDM(buf_out, drops, amAMOUNT, n, m);
#define _06_01_ENCODE_TL_AMOUNT_GUARDM(buf_out, drops, n, m) \
    ENCODE_TL_AMOUNT_GUARDM(buf_out, drops, n, m);

/////////// Macros to prepare a simple transaction with memos. ///////////

// Transaction fee offsets.
#define PAYMENT_SIMPLE_FEE_OFFSET 44
#define PAYMENT_SIMPLE_TRUSTLINE_FEE_OFFSET 84

#define SET_TRANSACTION_FEE(buf_out_master, offset)        \
    {                                                      \
        int64_t fee = etxn_fee_base(SBUF(buf_out_master)); \
        uint8_t *fee_ptr = buf_out_master + offset;        \
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);             \
    }

#define POPULATE_PAYMENT_SIMPLE_COMMON(buf_out, drops_amount_raw, drops_fee_raw, to_address) \
    {                                                                                        \
        uint8_t acc[20];                                                                     \
        uint64_t drops_amount = (drops_amount_raw);                                          \
        uint64_t drops_fee = (drops_fee_raw);                                                \
        uint32_t cls = (uint32_t)ledger_seq();                                               \
        hook_account(SBUF(acc));                                                             \
        _01_02_ENCODE_TT(buf_out, ttPAYMENT);              /* uint16  | size   3 */          \
        _02_02_ENCODE_FLAGS(buf_out, tfCANONICAL);         /* uint32  | size   5 */          \
        _02_03_ENCODE_TAG_SRC(buf_out, 0);                 /* uint32  | size   5 */          \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                /* uint32  | size   5 */          \
        _02_14_ENCODE_TAG_DST(buf_out, 0);                 /* uint32  | size   5 */          \
        _02_26_ENCODE_FLS(buf_out, cls + 1);               /* uint32  | size   6 */          \
        _02_27_ENCODE_LLS(buf_out, cls + 5);               /* uint32  | size   6 */          \
        _06_01_ENCODE_DROPS_AMOUNT(buf_out, drops_amount); /* amount  | size   9 */          \
        _06_08_ENCODE_DROPS_FEE(buf_out, drops_fee);       /* amount  | size   9 */          \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);        /* pk      | size  35 */          \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);           /* account | size  22 */          \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address);    /* account | size  22 */          \
    }

#define PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_SIZE(type_len, format_len, data_len) \
    ((type_len + (type_len <= 192 ? 2 : 3) + format_len + (format_len <= 192 ? 2 : 3) + data_len + (data_len <= 192 ? 2 : 3)) + 270 + 4)
#define PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_M(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, type, type_len, format, format_len, data, data_len, m) \
    {                                                                                                                                                             \
        uint8_t *buf_out = buf_out_master;                                                                                                                        \
        POPULATE_PAYMENT_SIMPLE_COMMON(buf_out, drops_amount_raw, drops_fee_raw, to_address);                                                                     \
        _0F_09_ENCODE_MEMOS_SINGLE_GUARDM(buf_out, type, type_len, format, format_len, data, data_len, 1, m);                                                     \
        etxn_details((uint32_t)buf_out, 138); /* emitdet | size 138 */                                                                                            \
    }

#define PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_SIZE(type1_len, format1_len, data1_len, type2_len, format2_len, data2_len)                   \
    ((type2_len + (type2_len <= 192 ? 2 : 3) + format2_len + (format2_len <= 192 ? 2 : 3) + data2_len + (data2_len <= 192 ? 2 : 3)) + \
     (type1_len + (type1_len <= 192 ? 2 : 3) + format1_len + (format1_len <= 192 ? 2 : 3) + data1_len + (data1_len <= 192 ? 2 : 3)) + 270 + 6)
#define PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_M(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, type1, type1_len, format1, format1_len, data1, data1_len, type2, type2_len, format2, format2_len, data2, data2_len, m) \
    {                                                                                                                                                                                                                          \
        uint8_t *buf_out = buf_out_master;                                                                                                                                                                                     \
        POPULATE_PAYMENT_SIMPLE_COMMON(buf_out, drops_amount_raw, drops_fee_raw, to_address);                                                                                                                                  \
        _0F_09_ENCODE_MEMOS_DUO_GUARDM(buf_out, type1, type1_len, format1, format1_len, data1, data1_len, type2, type2_len, format2, format2_len, data2, data2_len, 1, m);                                                     \
        etxn_details((uint32_t)buf_out, 138); /* emitdet | size 138 */                                                                                                                                                         \
    }

/////////// Macros to prepare a trustline transaction with memos. ///////////

#define POPULATE_PAYMENT_SIMPLE_TRUSTLINE_COMMON_GUARDM(buf_out, tlamt, drops_fee_raw, to_address, n, m) \
    {                                                                                                    \
        uint8_t acc[20];                                                                                 \
        uint64_t drops_fee = (drops_fee_raw);                                                            \
        uint32_t cls = (uint32_t)ledger_seq();                                                           \
        hook_account(SBUF(acc));                                                                         \
        _01_02_ENCODE_TT(buf_out, ttPAYMENT);                 /* uint16  | size   3 */                   \
        _02_02_ENCODE_FLAGS(buf_out, tfCANONICAL);            /* uint32  | size   5 */                   \
        _02_03_ENCODE_TAG_SRC(buf_out, 0);                    /* uint32  | size   5 */                   \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                   /* uint32  | size   5 */                   \
        _02_14_ENCODE_TAG_DST(buf_out, 0);                    /* uint32  | size   5 */                   \
        _02_26_ENCODE_FLS(buf_out, cls + 1);                  /* uint32  | size   6 */                   \
        _02_27_ENCODE_LLS(buf_out, cls + 5);                  /* uint32  | size   6 */                   \
        _06_01_ENCODE_TL_AMOUNT_GUARDM(buf_out, tlamt, n, m); /* amount  | size  49 */                   \
        _06_08_ENCODE_DROPS_FEE(buf_out, drops_fee);          /* amount  | size   9 */                   \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);           /* pk      | size  35 */                   \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);              /* account | size  22 */                   \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address);       /* account | size  22 */                   \
    }

#define PREPARE_PAYMENT_SIMPLE_TRUSTLINE_GUARD(buf_out_master, tlamt, drops_fee_raw, to_address, n)       \
    {                                                                                                     \
        uint8_t *buf_out = buf_out_master;                                                                \
        POPULATE_PAYMENT_SIMPLE_TRUSTLINE_COMMON_GUARDM(buf_out, tlamt, drops_fee_raw, to_address, n, 1); \
        etxn_details((uint32_t)buf_out, 138); /* emitdet | size 138 */                                    \
    }

#define PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(type_len, format_len, data_len) \
    ((type_len + (type_len <= 192 ? 2 : 3) + format_len + (format_len <= 192 ? 2 : 3) + data_len + (data_len <= 192 ? 2 : 3)) + 309 + 4)
#define PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_M(buf_out_master, tlamt, drops_fee_raw, to_address, type, type_len, format, format_len, data, data_len, m) \
    {                                                                                                                                                            \
        uint8_t *buf_out = buf_out_master;                                                                                                                       \
        POPULATE_PAYMENT_SIMPLE_TRUSTLINE_COMMON_GUARDM(buf_out, tlamt, drops_fee_raw, to_address, 1, 1);                                                        \
        _0F_09_ENCODE_MEMOS_SINGLE_GUARDM(buf_out, type, type_len, format, format_len, data, data_len, 1, m);                                                    \
        etxn_details((uint32_t)buf_out, 138); /* emitdet | size 138 */                                                                                           \
    }

/////////// Macros to prepare a check with memos. ///////////

#define POPULATE_SIMPLE_CHECK_COMMON_GUARDM(buf_out, tlamt, drops_fee_raw, to_address, n, m) \
    {                                                                                        \
        uint8_t acc[20];                                                                     \
        uint64_t drops_fee = (drops_fee_raw);                                                \
        uint32_t cls = (uint32_t)ledger_seq();                                               \
        hook_account(SBUF(acc));                                                             \
        _01_02_ENCODE_TT(buf_out, ttCHECK_CREATE);             /* uint16  | size   3 */      \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                    /* uint32  | size   5 */      \
        _02_26_ENCODE_FLS(buf_out, cls + 1);                   /* uint32  | size   6 */      \
        _02_27_ENCODE_LLS(buf_out, cls + 5);                   /* uint32  | size   6 */      \
        _06_09_ENCODE_TL_SENDMAX_GUARDM(buf_out, tlamt, n, m); /* amount  | size  49 */      \
        _06_08_ENCODE_DROPS_FEE(buf_out, drops_fee);           /* amount  | size   9 */      \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);            /* pk      | size  35 */      \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);               /* account | size  22 */      \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address);        /* account | size  22 */      \
    }

#define PREPARE_SIMPLE_CHECK_MEMOS_SINGLE_SIZE(type_len, format_len, data_len) \
    ((type_len + (type_len <= 192 ? 2 : 3) + format_len + (format_len <= 192 ? 2 : 3) + data_len + (data_len <= 192 ? 2 : 3)) + 295 + 4)
#define PREPARE_SIMPLE_CHECK_MEMOS_SINGLE_GUARDM(buf_out_master, tlamt, drops_fee_raw, to_address, type, type_len, format, format_len, data, data_len, n, m) \
    {                                                                                                                                                        \
        uint8_t *buf_out = buf_out_master;                                                                                                                   \
        POPULATE_SIMPLE_CHECK_COMMON_GUARDM(buf_out, tlamt, drops_fee_raw, to_address, n, m);                                                                \
        _0F_09_ENCODE_MEMOS_SINGLE_GUARDM(buf_out, type, type_len, format, format_len, data, data_len, n, m + 1);                                            \
        etxn_details((uint32_t)buf_out, 138); /* emitdet | size 138 */                                                                                       \
    }

/////////// Macro to prepare a trustline. ///////////

#define PREPARE_SIMPLE_TRUSTLINE_SIZE 278
#define PREPARE_SIMPLE_TRUSTLINE(buf_out_master, tlamt, drops_fee_raw)        \
    {                                                                         \
        uint8_t *buf_out = buf_out_master;                                    \
        uint8_t acc[20];                                                      \
        uint64_t drops_fee = (drops_fee_raw);                                 \
        uint32_t cls = (uint32_t)ledger_seq();                                \
        hook_account(SBUF(acc));                                              \
        _01_02_ENCODE_TT(buf_out, ttTRUST_SET);      /* uint16  | size   3 */ \
        _02_02_ENCODE_FLAGS(buf_out, tfSetNoRipple); /* uint32  | size   5 */ \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);          /* uint32  | size   5 */ \
        _02_26_ENCODE_FLS(buf_out, cls + 1);         /* uint32  | size   6 */ \
        _02_27_ENCODE_LLS(buf_out, cls + 5);         /* uint32  | size   6 */ \
        ENCODE_TL(buf_out, tlamt, amLIMITAMOUNT);    /* amount  | size  49 */ \
        _06_08_ENCODE_DROPS_FEE(buf_out, drops_fee); /* amount  | size   9 */ \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);  /* pk      | size  35 */ \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);     /* account | size  22 */ \
        etxn_details((uint32_t)buf_out, 138);        /* emitdet | size 138 */ \
    }

#define GENERATE_NFT_TOKEN_ID_GUARD(token_id, transaction_fee, accid, taxon, token_seq, n) \
    {                                                                                      \
        UINT16_TO_BUF(token_id, tfTransferable);                                           \
        UINT16_TO_BUF(token_id + 2, transaction_fee);                                      \
        COPY_BUF_GUARD(token_id, 4, accid, 0, 20, n);                                      \
        UINT32_TO_BUF(token_id + 24, taxon ^ ((NFT_TAXON_M * token_seq) + NFT_TAXON_C));   \
        UINT32_TO_BUF(token_id + 28, token_seq);                                           \
    }

#define GENERATE_NFT_TOKEN_ID(token_id, transaction_fee, accid, taxon, token_seq) \
    GENERATE_NFT_TOKEN_ID_GUARD(token_id, transaction_fee, accid, taxon, token_seq, 1)

/////////// Macro to prepare a nft mint. ///////////

#define PREPARE_NFT_MINT_SIZE(uri_len) \
    (uri_len + 240)
// Preapare a transferable NFT mint transaction.
// Note - Only the transferable flag will be set.
#define PREPARE_NFT_MINT(buf_out_master, transfer_fee, taxon, uri, uri_len)           \
    {                                                                                 \
        uint8_t *buf_out = buf_out_master;                                            \
        uint8_t acc[20];                                                              \
        uint32_t cls = (uint32_t)ledger_seq();                                        \
        hook_account(SBUF(acc));                                                      \
        _01_02_ENCODE_TT(buf_out, ttNFT_MINT);        /* uint16  | size   3 */        \
        _01_04_ENCODE_TF(buf_out, transfer_fee);      /* uint16  | size   3 */        \
        _02_02_ENCODE_FLAGS(buf_out, tfTransferable); /* uint32  | size   5 */        \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);           /* uint32  | size   5 */        \
        _02_26_ENCODE_FLS(buf_out, cls + 1);          /* uint32  | size   6 */        \
        _02_27_ENCODE_LLS(buf_out, cls + 5);          /* uint32  | size   6 */        \
        _02_42_ENCODE_TXON(buf_out, taxon);           /* uint32  | size   6 */        \
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

/////////// Macros to prepare evernode realated transactions. ///////////
#define PREPARE_PAYMENT_FOUNDATION_RETURN_SIZE \
    (PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(19, 0, 0))
#define PREPARE_PAYMENT_FOUNDATION_RETURN(buf_out_master, tlamt, drops_fee_raw, to_address)                                                                         \
    {                                                                                                                                                               \
        PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_M(buf_out_master, tlamt, drops_fee_raw, to_address, FOUNDATION_REFUND_50, 19, empty_ptr, 0, empty_ptr, 0, 1); \
    }

#define PREPARE_PAYMENT_REDEEM_SIZE(redeem_data_len, origin_data_len) \
    (PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_SIZE(9, 6, redeem_data_len, 15, 3, origin_data_len))
#define PREPARE_PAYMENT_REDEEM_M(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, redeem_data, redeem_data_len, origin_data, origin_data_len, m)                                                                           \
    {                                                                                                                                                                                                                                  \
        PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_M(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, REDEEM, 9, FORMAT_BASE64, 6, redeem_data, redeem_data_len, REDEEM_ORIGIN, 15, FORMAT_HEX, 3, origin_data, origin_data_len, m); \
    }

#define PREPARE_PAYMENT_REDEEM_RESP_SIZE(redeem_resp_len, redeem_ref_len, is_success) \
    (PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_SIZE((is_success ? 16 : 14), (is_success ? 6 : 9), redeem_resp_len, 12, 3, redeem_ref_len))
#define PREPARE_PAYMENT_REDEEM_RESP_M(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, redeem_resp_ptr, redeem_resp_len, redeem_ref_ptr, redeem_ref_len, is_success, m)                                                                    \
    {                                                                                                                                                                                                                                                  \
        if (is_success)                                                                                                                                                                                                                                \
        {                                                                                                                                                                                                                                              \
            PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_M(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, REDEEM_SUCCESS, 16, FORMAT_BASE64, 6, redeem_resp_ptr, redeem_resp_len, REDEEM_REF, 12, FORMAT_HEX, 3, redeem_ref_ptr, redeem_ref_len, m); \
        }                                                                                                                                                                                                                                              \
        else                                                                                                                                                                                                                                           \
        {                                                                                                                                                                                                                                              \
            PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_M(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, REDEEM_ERROR, 14, FORMAT_JSON, 9, redeem_resp_ptr, redeem_resp_len, REDEEM_REF, 12, FORMAT_HEX, 3, redeem_ref_ptr, redeem_ref_len, m);     \
        }                                                                                                                                                                                                                                              \
    }

#define PREPARE_PAYMENT_REWARD_SIZE \
    (PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(9, 0, 0))
#define PREPARE_PAYMENT_REWARD_M(buf_out_master, tlamt, drops_fee_raw, to_address, m)                                                                \
    {                                                                                                                                                \
        PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_M(buf_out_master, tlamt, drops_fee_raw, to_address, REWARD, 9, empty_ptr, 0, empty_ptr, 0, m); \
    }

#define PREPARE_PAYMENT_REFUND_SUCCESS_SIZE \
    (PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(16, 3, 64))
#define PREPARE_PAYMENT_REFUND_SUCCESS_M(buf_out_master, tlamt, drops_fee_raw, to_address, refund_ptr, redeem_ptr, m)                                           \
    {                                                                                                                                                           \
        uint8_t memo_data[64];                                                                                                                                  \
        COPY_BUF_GUARDM(memo_data, 0, refund_ptr, 0, 32, 1, 4);                                                                                                 \
        COPY_BUF_GUARDM(memo_data, 32, redeem_ptr, 0, 32, 1, 5);                                                                                                \
        PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_M(buf_out_master, tlamt, drops_fee_raw, to_address, REFUND_SUCCESS, 16, FORMAT_HEX, 3, memo_data, 64, m); \
    }

#define PREPARE_PAYMENT_REFUND_ERROR_SIZE \
    (PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_SIZE(14, 3, 32))
#define PREPARE_PAYMENT_REFUND_ERROR_M(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, refund_ptr, m)                                              \
    {                                                                                                                                                           \
        PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_M(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, REFUND_ERROR, 14, FORMAT_HEX, 3, refund_ptr, 32, m); \
    }

#define PREPARE_AUDIT_CHECK_SIZE \
    (PREPARE_SIMPLE_CHECK_MEMOS_SINGLE_SIZE(18, 0, 0))
#define PREPARE_AUDIT_CHECK_GUARDM(buf_out_master, tlamt, drops_fee_raw, to_address, n, m)                                                                  \
    {                                                                                                                                                       \
        PREPARE_SIMPLE_CHECK_MEMOS_SINGLE_GUARDM(buf_out_master, tlamt, drops_fee_raw, to_address, AUDIT_ASSIGNMENT, 18, empty_ptr, 0, empty_ptr, 0, n, m); \
    }

/////////// Macros for common logics. ///////////
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

// Adds the given amount to the reward pool.
#define ADD_TO_REWARD_POOL(float_amount)                                         \
    {                                                                            \
        /* Take the current reward pool amount from the config. */               \
        uint8_t reward_pool_buf[8] = {0};                                        \
        int64_t reward_pool = 0;                                                 \
        if (state(SBUF(reward_pool_buf), SBUF(STK_REWARD_POOL)) != DOESNT_EXIST) \
            reward_pool = INT64_FROM_BUF(reward_pool_buf);                       \
        reward_pool = float_sum(reward_pool, float_amount);                      \
        /* Update the last accumulated moment state. */                          \
        INT64_TO_BUF(reward_pool_buf, reward_pool);                              \
        if (state_set(SBUF(reward_pool_buf), SBUF(STK_REWARD_POOL)) < 0)         \
            rollback(SBUF("Evernode: Could not update the reward pool."), 1);    \
    }

#define GET_MOMENT_START_INDEX_MOMENT_BASE_SIZE_GIVEN(cur_moment_start_idx, cur_ledger_seq, moment_base_idx, moment_size) \
    {                                                                                                                     \
        uint64_t relative_n = (cur_ledger_seq - moment_base_idx) / moment_size;                                           \
        cur_moment_start_idx = moment_base_idx + (relative_n * moment_size);                                              \
    }

#define GET_MOMENT_START_INDEX_MOMENT_SIZE_GIVEN(cur_moment_start_idx, cur_ledger_seq, moment_size)                             \
    {                                                                                                                           \
        uint64_t moment_base_idx;                                                                                               \
        GET_CONF_VALUE(moment_base_idx, 0, STK_MOMENT_BASE_IDX, "Evernode: Could not set default state for moment base idx.");  \
        GET_MOMENT_START_INDEX_MOMENT_BASE_SIZE_GIVEN(cur_moment_start_idx, cur_ledger_seq, moment_base_idx, conf_moment_size); \
    }

#define IS_HOST_ACTIVE_MOMENT_IDX_SIZE_GIVEN(is_active, host_addr_buf, cur_moment_start_idx, moment_size)                                                              \
    {                                                                                                                                                                  \
        uint16_t conf_host_heartbeat_freq;                                                                                                                             \
        GET_CONF_VALUE(conf_host_heartbeat_freq, DEF_HOST_HEARTBEAT_FREQ, CONF_HOST_HEARTBEAT_FREQ, "Evernode: Could not set default state for host heartbeat freq."); \
        uint8_t *host_hearbeat_ledger_idx_ptr = &host_addr_buf[HOST_HEARTBEAT_LEDGER_IDX_OFFSET];                                                                      \
        int64_t last_hearbeat_ledger_idx = INT64_FROM_BUF(host_hearbeat_ledger_idx_ptr);                                                                               \
        if (cur_moment_start_idx > (conf_host_heartbeat_freq * moment_size))                                                                                           \
            is_active = (last_hearbeat_ledger_idx >= (cur_moment_start_idx - (conf_host_heartbeat_freq * moment_size)));                                               \
        else                                                                                                                                                           \
            is_active = (last_hearbeat_ledger_idx > 0);                                                                                                                \
    }

#define IS_HOST_ACTIVE(is_active, host_addr_buf, cur_ledger_seq)                                                                       \
    {                                                                                                                                  \
        uint16_t conf_moment_size;                                                                                                     \
        GET_CONF_VALUE(conf_moment_size, DEF_MOMENT_SIZE, CONF_MOMENT_SIZE, "Evernode: Could not set default state for moment size."); \
        uint64_t cur_moment_start_idx;                                                                                                 \
        GET_MOMENT_START_INDEX_MOMENT_SIZE_GIVEN(cur_moment_start_idx, cur_ledger_seq, conf_moment_size);                              \
        IS_HOST_ACTIVE_MOMENT_IDX_SIZE_GIVEN(is_active, host_addr_buf, cur_moment_start_idx, conf_moment_size);                        \
    }

#define EMIT_AUDIT_CHECK_GUARDM(cur_moment_start_idx, moment_seed_buf, min_redeem, host_addr, host_addr_buf, to_addr, n, m) \
    {                                                                                                                       \
        uint8_t *host_token_ptr = &host_addr_buf[HOST_TOKEN_OFFSET];                                                        \
        trace(SBUF("Hosting token"), host_token_ptr, 3, 1);                                                                 \
        /* If host is already assigned for audit within this moment we won't reward again. */                               \
        if (UINT64_FROM_BUF(&host_addr_buf[HOST_AUDIT_IDX_OFFSET]) == cur_moment_start_idx)                                 \
            rollback(SBUF("Evernode: Picked host is already assigned for audit within this moment."), 1);                   \
        int64_t token_limit = float_set(0, min_redeem);                                                                     \
        uint8_t amt_out[AMOUNT_BUF_SIZE];                                                                                   \
        SET_AMOUNT_OUT_GUARDM(amt_out, host_token_ptr, host_addr, token_limit, n, m);                                       \
        /* Finally create the outgoing txn. */                                                                              \
        uint8_t txn_out[PREPARE_AUDIT_CHECK_SIZE];                                                                          \
        PREPARE_AUDIT_CHECK_GUARDM(txn_out, amt_out, 0, to_addr, n, m + 1);                                                 \
        SET_TRANSACTION_FEE(txn_out, SIMPLE_CHECK_FEE_OFFSET);                                                              \
        uint8_t emithash[HASH_SIZE];                                                                                        \
        if (emit(SBUF(emithash), SBUF(txn_out)) < 0)                                                                        \
            rollback(SBUF("Evernode: Emitting hosting token check failed."), 1);                                            \
        trace(SBUF("emit hash: "), SBUF(emithash), 1);                                                                      \
        /* Update the host's audit assigned state. */                                                                       \
        COPY_BUF_GUARDM(host_addr_buf, HOST_AUDIT_IDX_OFFSET, moment_seed_buf, 0, 8, n, m + 5);                             \
        COPY_BUF_GUARDM(host_addr_buf, HOST_AUDITOR_OFFSET, to_addr, 0, 20, n, m + 6);                                      \
    }

enum LedgerEntryType
{
    ltNFTOKEN_PAGE = 0x0050
};

const uint8_t page_mask[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

#define GET_NFT(account, nft_id, nft_issuer, nft_uri, nft_uri_len, nft_taxon, nft_flags)               \
    {                                                                                                  \
        uint8_t lo_keylet[34];                                                                         \
        uint8_t buf[32] = {0};                                                                         \
        COPY_BUF_GUARDM(buf, 0, account, 0, 20, 1, 1);                                                 \
        lo_keylet[0] = (ltNFTOKEN_PAGE >> 8) & 0xFFU;                                                  \
        lo_keylet[1] = (ltNFTOKEN_PAGE >> 0) & 0xFFU;                                                  \
        COPY_BUF_GUARDM(lo_keylet, 2, buf, 0, 32, 1, 2);                                               \
                                                                                                       \
        uint8_t id_keylet[34] = {0};                                                                   \
        id_keylet[0] = (ltNFTOKEN_PAGE >> 8) & 0xFFU;                                                  \
        id_keylet[1] = (ltNFTOKEN_PAGE >> 0) & 0xFFU;                                                  \
        for (int i = 0; GUARDM(32, 3), i < 32; ++i)                                                    \
            id_keylet[2 + i] = (lo_keylet[2 + i] & ~page_mask[i]) + (nft_id[i] & page_mask[i]);        \
                                                                                                       \
        uint8_t hi_keylet[34];                                                                         \
        uint8_t id[32];                                                                                \
        COPY_BUF_GUARDM(id, 0, account, 0, 20, 1, 4);                                                  \
        COPY_BUF_GUARDM(id, 20, page_mask, 20, 34, 1, 5);                                              \
        hi_keylet[0] = (ltNFTOKEN_PAGE >> 8) & 0xFFU;                                                  \
        hi_keylet[1] = (ltNFTOKEN_PAGE >> 0) & 0xFFU;                                                  \
        COPY_BUF_GUARDM(hi_keylet, 2, id, 0, 32, 1, 6);                                                \
                                                                                                       \
        uint8_t nft_keylet[34];                                                                        \
        if (ledger_keylet(SBUF(nft_keylet), SBUF(id_keylet), SBUF(hi_keylet)) != 34)                   \
            rollback(SBUF("Evernode: Could not generate the ledger nft keylet."), 10);                 \
                                                                                                       \
        int64_t nfts_slot = slot_set(SBUF(nft_keylet), 0);                                             \
        if (nfts_slot < 0)                                                                             \
            rollback(SBUF("Evernode: Could not set ledger nft keylet in slot"), 10);                   \
                                                                                                       \
        nfts_slot = slot_subfield(nfts_slot, sfNFTokens, 0);                                           \
        if (nfts_slot < 0)                                                                             \
            rollback(SBUF("Evernode: Could not find sfNFTokens on ledger nft keylet"), 1);             \
                                                                                                       \
        for (int i = 0; GUARDM(32, 7), i < 32; ++i)                                                    \
        {                                                                                              \
            int64_t nft_slot = slot_subarray(nfts_slot, i, 0);                                         \
            if (nft_slot >= 0)                                                                         \
            {                                                                                          \
                int64_t id_slot = slot_subfield(nft_slot, sfNFTokenID, 0);                             \
                uint8_t cur_id[32] = {0};                                                              \
                if (id_slot >= 0 && slot(SBUF(cur_id), id_slot) == 32)                                 \
                {                                                                                      \
                    int equal = 0;                                                                     \
                    BUFFER_EQUAL_GUARDM(equal, cur_id, 32, nft_id, 32, 32, 8);                         \
                    if (equal)                                                                         \
                    {                                                                                  \
                        int64_t issuer_slot = slot_subfield(nft_slot, sfIssuer, 0);                    \
                        int64_t uri_slot = slot_subfield(nft_slot, sfURI, 0);                          \
                        int64_t taxon_slot = slot_subfield(nft_slot, sfNFTokenTaxon, 0);               \
                        int64_t flags_slot = slot_subfield(nft_slot, sfFlags, 0);                      \
                                                                                                       \
                        slot(SBUF(nft_issuer), issuer_slot);                                           \
                        nft_uri_len = slot(SBUF(nft_uri), uri_slot);                                   \
                        uint8_t taxon_buf[4];                                                          \
                        slot(SBUF(taxon_buf), taxon_slot);                                             \
                        nft_taxon = UINT32_FROM_BUF(taxon_buf);                                        \
                        uint8_t flags_buf[4];                                                          \
                        slot(SBUF(flags_buf), flags_slot);                                             \
                        nft_flags = UINT32_FROM_BUF(flags_buf);                                        \
                        break;                                                                         \
                    }                                                                                  \
                }                                                                                      \
            }                                                                                          \
        }                                                                                              \
        nft_uri_len = nft_uri[0];                                                                      \
        COPY_BUF_NON_CONST_LEN_GUARDM(nft_uri, 0, nft_uri, 1, nft_uri_len + 1, sizeof(nft_uri), 1, 9); \
    }

#endif