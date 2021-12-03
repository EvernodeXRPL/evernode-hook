#ifndef EVERNODE_INCLUDED
#define EVERNODE_INCLUDED 1

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

// Checks for EVR currency issued by hook account.
#define IS_EVR(is_evr, amount_buffer, hook_accid)      \
    is_evr = 1;                                        \
    for (int i = 0; GUARD(20), i < 20; ++i)            \
    {                                                  \
        if (amount_buffer[i + 8] != evr_currency[i] || \
            amount_buffer[i + 28] != hook_accid[i])    \
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

#define IS_BUF_EMPTY(is_empty, buf)                           \
    is_empty = 1;                                             \
    for (int i = 0; GUARD(sizeof(buf)), i < sizeof(buf); ++i) \
    {                                                         \
        if (buf[i] != 0)                                      \
        {                                                     \
            is_empty = 0;                                     \
            break;                                            \
        }                                                     \
    }

#define GET_CONF_VALUE(value, def_value, key, error_buf)         \
    {                                                            \
        uint8_t size = sizeof(value);                            \
        uint8_t value_buf[size];                                 \
        int64_t state_res = state(SBUF(value_buf), SBUF(key));   \
        switch (size)                                            \
        {                                                        \
        case 2:                                                  \
            if (state_res == DOESNT_EXIST)                       \
            {                                                    \
                value = def_value;                               \
                UINT16_TO_BUF(value_buf, value);                 \
            }                                                    \
            else                                                 \
                value = UINT16_FROM_BUF(value_buf);              \
            break;                                               \
        case 4:                                                  \
            if (state_res == DOESNT_EXIST)                       \
            {                                                    \
                value = def_value;                               \
                UINT32_TO_BUF(value_buf, value);                 \
            }                                                    \
            else                                                 \
                value = UINT32_FROM_BUF(value_buf);              \
            break;                                               \
        case 8:                                                  \
            if (state_res == DOESNT_EXIST)                       \
            {                                                    \
                value = def_value;                               \
                UINT64_TO_BUF(value_buf, value);                 \
            }                                                    \
            else                                                 \
                value = UINT64_FROM_BUF(value_buf);              \
            break;                                               \
        default:                                                 \
            rollback(SBUF("Evernode: Invalid state value."), 1); \
            break;                                               \
        }                                                        \
        if (state_res == DOESNT_EXIST)                           \
        {                                                        \
            if (state_set(SBUF(value_buf), SBUF(key)) < 0)       \
                rollback(SBUF(error_buf), 1);                    \
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

#define COPY_BUF(lhsbuf, lhsbuf_spos, rhsbuf, rhsbuf_spos, len) \
    for (int i = 0; GUARD(len), i < len; ++i)                   \
        lhsbuf[lhsbuf_spos + i] = rhsbuf[rhsbuf_spos + i];

#define COPY_BUFM(lhsbuf, lhsbuf_spos, rhsbuf, rhsbuf_spos, len, n) \
    for (int i = 0; GUARDM(len, n), i < len; ++i)                   \
        lhsbuf[lhsbuf_spos + i] = rhsbuf[rhsbuf_spos + i];

// If host count state does not exist, set host count to 0.
#define GET_HOST_COUNT(host_count_buf, host_count)                             \
    {                                                                          \
        CLEARBUF(host_count_buf);                                              \
        host_count = 0;                                                        \
        if (state(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) != DOESNT_EXIST) \
            host_count = UINT32_FROM_BUF(host_count_buf);                      \
    }

// We need to dump the iou amount into a buffer.
// by supplying -1 as the fieldcode we tell float_sto not to prefix an actual STO header on the field.
#define SET_AMOUNT_OUT(amt_out, token, issuer, amount)                            \
    {                                                                             \
        uint8_t currency[20] = GET_TOKEN_CURRENCY(token);                         \
        if (float_sto(SBUF(amt_out), SBUF(currency), issuer, 20, amount, -1) < 0) \
            rollback(SBUF("Evernode: Could not dump token amount into sto"), 1);  \
        for (int i = 0; GUARD(20), i < 20; ++i)                                   \
        {                                                                         \
            amt_out[i + 28] = issuer[i];                                          \
            amt_out[i + 8] = currency[i];                                         \
        }                                                                         \
        if (amount == 0)                                                          \
            amt_out[0] = amt_out[0] & 0b10111111; /* Set the sign bit to 0.*/     \
    }

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
/**************************MEMO related MACROS*****************************/
/**************************************************************************/

#define ENCODE_FIELDS_SIZE 1U
#define ENCODE_FIELDS(buf_out, type, field) \
    {                                       \
        uint8_t uf = field;                 \
        buf_out[0] = type + (uf & 0x0FU);   \
        buf_out += ENCODE_FIELDS_SIZE;      \
    }

#define ENCODE_STI_VL_COMMON(buf_out, data, data_len, field, n)                   \
    {                                                                             \
        uint8_t *ptr = (uint8_t *)&data;                                          \
        uint8_t uf = field;                                                       \
        buf_out[0] = 0x70U + (uf & 0x0FU);                                        \
        if (data_len <= 192) /*Data legth is represented with 2 bytes if (>192)*/ \
        {                                                                         \
            buf_out[1] = data_len;                                                \
            COPY_BUFM(buf_out, 2, ptr, 0, data_len, n);                           \
            buf_out += (2 + data_len);                                            \
        }                                                                         \
        else                                                                      \
        {                                                                         \
            buf_out[0] = 0x70U + (uf & 0x0FU);                                    \
            buf_out[1] = ((data_len - 193) / 256) + 193;                          \
            buf_out[2] = data_len - (((buf_out[1] - 193) * 256) + 193);           \
            COPY_BUFM(buf_out, 3, ptr, 0, data_len, n);                           \
            buf_out += (3 + data_len);                                            \
        }                                                                         \
    }

#define _07_XX_ENCODE_STI_VL_COMMON(buf_out, data, data_len, field, n) \
    ENCODE_STI_VL_COMMON(buf_out, data, data_len, field, n)

#define _0F_09_ENCODE_MEMOS_SINGLE(buf_out, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len)                                          \
    {                                                                                                                                                \
        ENCODE_FIELDS(buf_out, ARRAY, MEMOS); /*Arr Start*/                           /* uint32  | size   1 */                                       \
        ENCODE_FIELDS(buf_out, OBJECT, MEMO); /*Obj start*/                           /* uint32  | size   1 */                                       \
        _07_XX_ENCODE_STI_VL_COMMON(buf_out, type_ptr, type_len, MEMO_TYPE, 7);       /* STI_VL  | size   type_len + (type_len <= 192 ? 2 : 3)*/     \
        _07_XX_ENCODE_STI_VL_COMMON(buf_out, data_ptr, data_len, MEMO_DATA, 8);       /* STI_VL  | size   data_len + (data_len <= 192 ? 2 : 3)*/     \
        _07_XX_ENCODE_STI_VL_COMMON(buf_out, format_ptr, format_len, MEMO_FORMAT, 9); /* STI_VL  | size   format_len + (format_len <= 192 ? 2 : 3)*/ \
        ENCODE_FIELDS(buf_out, OBJECT, END); /*Obj end*/                              /* uint32  | size   1 */                                       \
        ENCODE_FIELDS(buf_out, ARRAY, END); /*Arr End*/                               /* uint32  | size   1 */                                       \
    }

#define _0F_09_ENCODE_MEMOS_DUO(buf_out, type1_ptr, type1_len, format1_ptr, format1_len, data1_ptr, data1_len, type2_ptr, type2_len, format2_ptr, format2_len, data2_ptr, data2_len) \
    {                                                                                                                                                                                \
        ENCODE_FIELDS(buf_out, ARRAY, MEMOS); /*Arr Start*/                              /* uint32  | size   1 */                                                                    \
        ENCODE_FIELDS(buf_out, OBJECT, MEMO); /*Obj start*/                              /* uint32  | size   1 */                                                                    \
        _07_XX_ENCODE_STI_VL_COMMON(buf_out, type1_ptr, type1_len, MEMO_TYPE, 7);        /* STI_VL  | size   type_len + (type_len <= 192 ? 2 : 3)*/                                  \
        _07_XX_ENCODE_STI_VL_COMMON(buf_out, data1_ptr, data1_len, MEMO_DATA, 8);        /* STI_VL  | size   data_len + (data_len <= 192 ? 2 : 3)*/                                  \
        _07_XX_ENCODE_STI_VL_COMMON(buf_out, format1_ptr, format1_len, MEMO_FORMAT, 9);  /* STI_VL  | size   format_len + (format_len <= 192 ? 2 : 3)*/                              \
        ENCODE_FIELDS(buf_out, OBJECT, END); /*Obj end*/                                 /* uint32  | size   1 */                                                                    \
        ENCODE_FIELDS(buf_out, OBJECT, MEMO); /*Obj start*/                              /* uint32  | size   1 */                                                                    \
        _07_XX_ENCODE_STI_VL_COMMON(buf_out, type2_ptr, type2_len, MEMO_TYPE, 10);       /* STI_VL  | size   type_len + (type_len <= 192 ? 2 : 3)*/                                  \
        _07_XX_ENCODE_STI_VL_COMMON(buf_out, data2_ptr, data2_len, MEMO_DATA, 11);       /* STI_VL  | size   data_len + (data_len <= 192 ? 2 : 3)*/                                  \
        _07_XX_ENCODE_STI_VL_COMMON(buf_out, format2_ptr, format2_len, MEMO_FORMAT, 12); /* STI_VL  | size   format_len + (format_len <= 192 ? 2 : 3)*/                              \
        ENCODE_FIELDS(buf_out, OBJECT, END); /*Obj end*/                                 /* uint32  | size   1 */                                                                    \
        ENCODE_FIELDS(buf_out, ARRAY, END); /*Arr End*/                                  /* uint32  | size   1 */                                                                    \
    }

/////////// Macros to prepare a simple transaction with memos. ///////////

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
    ((type_len + (type_len <= 192 ? 2 : 3) + format_len + (format_len <= 192 ? 2 : 3) + data_len + (data_len <= 192 ? 2 : 3)) + 237 + 4)
#define PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, type, type_len, format, format_len, data, data_len) \
    {                                                                                                                                                        \
        uint8_t *buf_out = buf_out_master;                                                                                                                   \
        POPULATE_PAYMENT_SIMPLE_COMMON(buf_out, drops_amount_raw, drops_fee_raw, to_address);                                                                \
        _0F_09_ENCODE_MEMOS_SINGLE(buf_out, type, type_len, format, format_len, data, data_len);                                                             \
        etxn_details((uint32_t)buf_out, 105); /* emitdet | size 105 */                                                                                       \
    }

#define PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_SIZE(type1_len, format1_len, data1_len, type2_len, format2_len, data2_len)                   \
    ((type2_len + (type2_len <= 192 ? 2 : 3) + format2_len + (format2_len <= 192 ? 2 : 3) + data2_len + (data2_len <= 192 ? 2 : 3)) + \
     (type1_len + (type1_len <= 192 ? 2 : 3) + format1_len + (format1_len <= 192 ? 2 : 3) + data1_len + (data1_len <= 192 ? 2 : 3)) + 237 + 6)
#define PREPARE_PAYMENT_SIMPLE_MEMOS_DUO(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, type1, type1_len, format1, format1_len, data1, data1_len, type2, type2_len, format2, format2_len, data2, data2_len) \
    {                                                                                                                                                                                                                     \
        uint8_t *buf_out = buf_out_master;                                                                                                                                                                                \
        POPULATE_PAYMENT_SIMPLE_COMMON(buf_out, drops_amount_raw, drops_fee_raw, to_address);                                                                                                                             \
        _0F_09_ENCODE_MEMOS_DUO(buf_out, type1, type1_len, format1, format1_len, data1, data1_len, type2, type2_len, format2, format2_len, data2, data2_len);                                                             \
        etxn_details((uint32_t)buf_out, 105); /* emitdet | size 105 */                                                                                                                                                    \
    }

/////////// Macros to prepare a trustline transaction with memos. ///////////

#define POPULATE_PAYMENT_SIMPLE_TRUSTLINE_COMMON(buf_out, tlamt, drops_fee_raw, to_address) \
    {                                                                                       \
        uint8_t acc[20];                                                                    \
        uint64_t drops_fee = (drops_fee_raw);                                               \
        uint32_t cls = (uint32_t)ledger_seq();                                              \
        hook_account(SBUF(acc));                                                            \
        _01_02_ENCODE_TT(buf_out, ttPAYMENT);           /* uint16  | size   3 */            \
        _02_02_ENCODE_FLAGS(buf_out, tfCANONICAL);      /* uint32  | size   5 */            \
        _02_03_ENCODE_TAG_SRC(buf_out, 0);              /* uint32  | size   5 */            \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);             /* uint32  | size   5 */            \
        _02_14_ENCODE_TAG_DST(buf_out, 0);              /* uint32  | size   5 */            \
        _02_26_ENCODE_FLS(buf_out, cls + 1);            /* uint32  | size   6 */            \
        _02_27_ENCODE_LLS(buf_out, cls + 5);            /* uint32  | size   6 */            \
        _06_01_ENCODE_TL_AMOUNT(buf_out, tlamt);        /* amount  | size  48 */            \
        _06_08_ENCODE_DROPS_FEE(buf_out, drops_fee);    /* amount  | size   9 */            \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);     /* pk      | size  35 */            \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);        /* account | size  22 */            \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address); /* account | size  22 */            \
    }

#define PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(type_len, format_len, data_len) \
    ((type_len + (type_len <= 192 ? 2 : 3) + format_len + (format_len <= 192 ? 2 : 3) + data_len + (data_len <= 192 ? 2 : 3)) + 276 + 4)
#define PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE(buf_out_master, tlamt, drops_fee_raw, to_address, type, type_len, format, format_len, data, data_len) \
    {                                                                                                                                                       \
        uint8_t *buf_out = buf_out_master;                                                                                                                  \
        POPULATE_PAYMENT_SIMPLE_TRUSTLINE_COMMON(buf_out, tlamt, drops_fee_raw, to_address);                                                                \
        _0F_09_ENCODE_MEMOS_SINGLE(buf_out, type, type_len, format, format_len, data, data_len);                                                            \
        etxn_details((uint32_t)buf_out, 105); /* emitdet | size 105 */                                                                                      \
    }

/////////// Macros to prepare a check with memos. ///////////

#define POPULATE_SIMPLE_CHECK_COMMON(buf_out, tlamt, drops_fee_raw, to_address)  \
    {                                                                            \
        uint8_t acc[20];                                                         \
        uint64_t drops_fee = (drops_fee_raw);                                    \
        uint32_t cls = (uint32_t)ledger_seq();                                   \
        hook_account(SBUF(acc));                                                 \
        _01_02_ENCODE_TT(buf_out, ttCHECK_CREATE);      /* uint16  | size   3 */ \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);             /* uint32  | size   5 */ \
        _02_26_ENCODE_FLS(buf_out, cls + 1);            /* uint32  | size   6 */ \
        _02_27_ENCODE_LLS(buf_out, cls + 5);            /* uint32  | size   6 */ \
        _06_09_ENCODE_TL_SENDMAX(buf_out, tlamt);       /* amount  | size  48 */ \
        _06_08_ENCODE_DROPS_FEE(buf_out, drops_fee);    /* amount  | size   9 */ \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);     /* pk      | size  35 */ \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);        /* account | size  22 */ \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address); /* account | size  22 */ \
    }

#define PREPARE_SIMPLE_CHECK_MEMOS_SINGLE_SIZE(type_len, format_len, data_len) \
    ((type_len + (type_len <= 192 ? 2 : 3) + format_len + (format_len <= 192 ? 2 : 3) + data_len + (data_len <= 192 ? 2 : 3)) + 261 + 4)
#define PREPARE_SIMPLE_CHECK_MEMOS_SINGLE(buf_out_master, tlamt, drops_fee_raw, to_address, type, type_len, format, format_len, data, data_len) \
    {                                                                                                                                           \
        uint8_t *buf_out = buf_out_master;                                                                                                      \
        POPULATE_SIMPLE_CHECK_COMMON(buf_out, tlamt, drops_fee_raw, to_address);                                                                \
        _0F_09_ENCODE_MEMOS_SINGLE(buf_out, type, type_len, format, format_len, data, data_len);                                                \
        etxn_details((uint32_t)buf_out, 105); /* emitdet | size 105 */                                                                          \
    }

/////////// Macro to prepare a trustline. ///////////

#define PREPARE_SIMPLE_TRUSTLINE_SIZE 245
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
        ENCODE_TL(buf_out, tlamt, amLIMITAMOUNT);    /* amount  | size  48 */ \
        _06_08_ENCODE_DROPS_FEE(buf_out, drops_fee); /* amount  | size   9 */ \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);  /* pk      | size  35 */ \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);     /* account | size  22 */ \
        etxn_details((uint32_t)buf_out, 105);        /* emitdet | size 105 */ \
    }

/////////// Macros to prepare evernode realated transactions. ///////////

#define PREPARE_PAYMENT_REDEEM_SIZE(redeem_data_len, origin_data_len) \
    (PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_SIZE(9, 6, redeem_data_len, 15, 3, origin_data_len))
#define PREPARE_PAYMENT_REDEEM(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, redeem_data, redeem_data_len, origin_data, origin_data_len)                                                                           \
    {                                                                                                                                                                                                                             \
        PREPARE_PAYMENT_SIMPLE_MEMOS_DUO(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, REDEEM, 9, FORMAT_BASE64, 6, redeem_data, redeem_data_len, REDEEM_ORIGIN, 15, FORMAT_HEX, 3, origin_data, origin_data_len); \
    }

#define PREPARE_PAYMENT_REDEEM_RESP_SIZE(redeem_resp_len, redeem_ref_len, is_success) \
    (PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_SIZE((is_success ? 16 : 14), (is_success ? 6 : 9), redeem_resp_len, 12, 3, redeem_ref_len))
#define PREPARE_PAYMENT_REDEEM_RESP(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, redeem_resp_ptr, redeem_resp_len, redeem_ref_ptr, redeem_ref_len, is_success)                                                                    \
    {                                                                                                                                                                                                                                             \
        if (is_success)                                                                                                                                                                                                                           \
        {                                                                                                                                                                                                                                         \
            PREPARE_PAYMENT_SIMPLE_MEMOS_DUO(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, REDEEM_SUCCESS, 16, FORMAT_BASE64, 6, redeem_resp_ptr, redeem_resp_len, REDEEM_REF, 12, FORMAT_HEX, 3, redeem_ref_ptr, redeem_ref_len); \
        }                                                                                                                                                                                                                                         \
        else                                                                                                                                                                                                                                      \
        {                                                                                                                                                                                                                                         \
            PREPARE_PAYMENT_SIMPLE_MEMOS_DUO(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, REDEEM_ERROR, 14, FORMAT_JSON, 9, redeem_resp_ptr, redeem_resp_len, REDEEM_REF, 12, FORMAT_HEX, 3, redeem_ref_ptr, redeem_ref_len);     \
        }                                                                                                                                                                                                                                         \
    }

#define PREPARE_PAYMENT_REWARD_SIZE \
    (PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(9, 0, 0))
#define PREPARE_PAYMENT_REWARD(buf_out_master, tlamt, drops_fee_raw, to_address)                                                                \
    {                                                                                                                                           \
        PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE(buf_out_master, tlamt, drops_fee_raw, to_address, REWARD, 9, empty_ptr, 0, empty_ptr, 0); \
    }

#define PREPARE_PAYMENT_REFUND_SUCCESS_SIZE \
    (PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE_SIZE(16, 3, 64))
#define PREPARE_PAYMENT_REFUND_SUCCESS(buf_out_master, tlamt, drops_fee_raw, to_address, refund_ptr, redeem_ptr)                                           \
    {                                                                                                                                                      \
        uint8_t memo_data[64];                                                                                                                             \
        COPY_BUFM(memo_data, 0, refund_ptr, 0, 32, 4);                                                                                                     \
        COPY_BUFM(memo_data, 32, redeem_ptr, 0, 32, 5);                                                                                                    \
        PREPARE_PAYMENT_SIMPLE_TRUSTLINE_MEMOS_SINGLE(buf_out_master, tlamt, drops_fee_raw, to_address, REFUND_SUCCESS, 16, FORMAT_HEX, 3, memo_data, 64); \
    }

#define PREPARE_PAYMENT_REFUND_ERROR_SIZE \
    (PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_SIZE(14, 3, 32))
#define PREPARE_PAYMENT_REFUND_ERROR(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, refund_ptr)                                              \
    {                                                                                                                                                      \
        PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, REFUND_ERROR, 14, FORMAT_HEX, 3, refund_ptr, 32); \
    }

#define PREPARE_AUDIT_CHECK_SIZE \
    (PREPARE_SIMPLE_CHECK_MEMOS_SINGLE_SIZE(18, 0, 0))
#define PREPARE_AUDIT_CHECK(buf_out_master, tlamt, drops_fee_raw, to_address, data, data_len)                                                  \
    {                                                                                                                                          \
        PREPARE_SIMPLE_CHECK_MEMOS_SINGLE(buf_out_master, tlamt, drops_fee_raw, to_address, AUDIT_ASSIGNMENT, 18, empty_ptr, 0, empty_ptr, 0); \
    }

#endif