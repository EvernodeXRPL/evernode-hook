/* This file includes deprecated memos which implemented for old hook.
In future these might be reusable, So we keep these as a seperate file. */

#ifndef EVERNODE_INCLUDED
#define EVERNODE_INCLUDED 1

#include "../evernode.h"
#include "../../lib/hookapi.h"

/**************************************************************************/
/**********************Guarded macro.h duplicates**************************/
/**************************************************************************/

#define ENCODE_TL_SENDMAX_GUARDM(buf_out, drops, n, m) \
    ENCODE_TL_GUARDM(buf_out, drops, amSENDMAX, n, m);
#define _06_09_ENCODE_TL_SENDMAX_GUARDM(buf_out, drops, n, m) \
    ENCODE_TL_SENDMAX_GUARDM(buf_out, drops, n, n);

/**************************************************************************/
/**********************Macros to prepare transactions**********************/
/**************************************************************************/

/////////// Macro to prepare a check. ///////////

#define PREPARE_SIMPLE_CHECK_SIZE 294
#define PREPARE_SIMPLE_CHECK(buf_out_master, tlamt, drops_fee_raw, to_address)          \
    {                                                                                   \
        uint8_t *buf_out = buf_out_master;                                              \
        uint8_t acc[20];                                                                \
        uint64_t drops_fee = (drops_fee_raw);                                           \
        uint32_t cls = (uint32_t)ledger_seq();                                          \
        hook_account(SBUF(acc));                                                        \
        _01_02_ENCODE_TT(buf_out, ttCHECK_CREATE);             /* uint16  | size   3 */ \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                    /* uint32  | size   5 */ \
        _02_26_ENCODE_FLS(buf_out, cls + 1);                   /* uint32  | size   6 */ \
        _02_27_ENCODE_LLS(buf_out, cls + 5);                   /* uint32  | size   6 */ \
        _06_09_ENCODE_TL_SENDMAX_GUARDM(buf_out, tlamt, 1, 1); /* amount  | size  48 */ \
        uint8_t *fee_ptr = buf_out;                                                     \
        _06_08_ENCODE_DROPS_FEE(buf_out, drops_fee);    /* amount  | size   9 */        \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);     /* pk      | size  35 */        \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);        /* account | size  22 */        \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address); /* account | size  22 */        \
        etxn_details((uint32_t)buf_out, 138);           /* emitdet | size 138 */        \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_SIMPLE_CHECK_SIZE);         \
        \ 
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                          \
    }

/////////// Macro to prepare a trustline. ///////////

#define PREPARE_SIMPLE_TRUSTLINE_SIZE 278
#define PREPARE_SIMPLE_TRUSTLINE(buf_out_master, tlamt, drops_fee_raw)              \
    {                                                                               \
        uint8_t *buf_out = buf_out_master;                                          \
        uint8_t acc[20];                                                            \
        uint64_t drops_fee = (drops_fee_raw);                                       \
        uint32_t cls = (uint32_t)ledger_seq();                                      \
        hook_account(SBUF(acc));                                                    \
        _01_02_ENCODE_TT(buf_out, ttTRUST_SET);      /* uint16  | size   3 */       \
        _02_02_ENCODE_FLAGS(buf_out, tfSetNoRipple); /* uint32  | size   5 */       \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);          /* uint32  | size   5 */       \
        _02_26_ENCODE_FLS(buf_out, cls + 1);         /* uint32  | size   6 */       \
        _02_27_ENCODE_LLS(buf_out, cls + 5);         /* uint32  | size   6 */       \
        ENCODE_TL(buf_out, tlamt, amLIMITAMOUNT);    /* amount  | size  49 */       \
        uint8_t *fee_ptr = buf_out;                                                 \
        _06_08_ENCODE_DROPS_FEE(buf_out, drops_fee); /* amount  | size   9 */       \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);  /* pk      | size  35 */       \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);     /* account | size  22 */       \
        etxn_details((uint32_t)buf_out, 138);        /* emitdet | size 138 */       \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_SIMPLE_TRUSTLINE_SIZE); \
        \ 
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                      \
    }

/**************************************************************************/
/*****************Macros to prepare transactions with memos****************/
/**************************************************************************/

/////////// Macros to prepare a simple payment with memos. ///////////

#define PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_SIZE(type_len, format_len, data_len) \
    ((type_len + (type_len <= 192 ? 2 : 3) + format_len + (format_len <= 192 ? 2 : 3) + data_len + (data_len <= 192 ? 2 : 3)) + PREPARE_PAYMENT_SIMPLE_SIZE + 4)
#define PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_M(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, dest_tag_raw, src_tag_raw, type, type_len, format, format_len, data, data_len, m) \
    {                                                                                                                                                                                        \
        uint8_t *buf_out = buf_out_master;                                                                                                                                                   \
        uint8_t acc[20];                                                                                                                                                                     \
        uint64_t drops_amount = (drops_amount_raw);                                                                                                                                          \
        uint32_t dest_tag = (dest_tag_raw);                                                                                                                                                  \
        uint32_t src_tag = (src_tag_raw);                                                                                                                                                    \
        uint32_t cls = (uint32_t)ledger_seq();                                                                                                                                               \
        hook_account(SBUF(acc));                                                                                                                                                             \
        _01_02_ENCODE_TT(buf_out, ttPAYMENT);              /* uint16  | size   3 */                                                                                                          \
        _02_02_ENCODE_FLAGS(buf_out, tfCANONICAL);         /* uint32  | size   5 */                                                                                                          \
        _02_03_ENCODE_TAG_SRC(buf_out, src_tag);           /* uint32  | size   5 */                                                                                                          \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                /* uint32  | size   5 */                                                                                                          \
        _02_14_ENCODE_TAG_DST(buf_out, dest_tag);          /* uint32  | size   5 */                                                                                                          \
        _02_26_ENCODE_FLS(buf_out, cls + 1);               /* uint32  | size   6 */                                                                                                          \
        _02_27_ENCODE_LLS(buf_out, cls + 5);               /* uint32  | size   6 */                                                                                                          \
        _06_01_ENCODE_DROPS_AMOUNT(buf_out, drops_amount); /* amount  | size   9 */                                                                                                          \
        uint8_t *fee_ptr = buf_out;                                                                                                                                                          \
        _06_08_ENCODE_DROPS_FEE(buf_out, 0);            /* amount  | size   9 */                                                                                                             \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);     /* pk      | size  35 */                                                                                                             \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);        /* account | size  22 */                                                                                                             \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address); /* account | size  22 */                                                                                                             \
        _0F_09_ENCODE_MEMOS_SINGLE_GUARDM(buf_out, type, type_len, format, format_len, data, data_len, 1, m);                                                                                \
        int64_t edlen = etxn_details((uint32_t)buf_out, PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_SIZE(type_len, format_len, data_len)); /* emitdet | size 1?? */                                  \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_PAYMENT_SIMPLE_MEMOS_SINGLE_SIZE(type_len, format_len, data_len));                                                               \
        \ 
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                                                                                                                               \
    }

#define PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_SIZE(type1_len, format1_len, data1_len, type2_len, format2_len, data2_len)                   \
    ((type2_len + (type2_len <= 192 ? 2 : 3) + format2_len + (format2_len <= 192 ? 2 : 3) + data2_len + (data2_len <= 192 ? 2 : 3)) + \
     (type1_len + (type1_len <= 192 ? 2 : 3) + format1_len + (format1_len <= 192 ? 2 : 3) + data1_len + (data1_len <= 192 ? 2 : 3)) + PREPARE_PAYMENT_SIMPLE_SIZE + 6)
#define PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_M(buf_out_master, drops_amount_raw, drops_fee_raw, to_address, dest_tag_raw, src_tag_raw, type1, type1_len, format1, format1_len, data1, data1_len, type2, type2_len, format2, format2_len, data2, data2_len, m) \
    {                                                                                                                                                                                                                                                     \
        uint8_t *buf_out = buf_out_master;                                                                                                                                                                                                                \
        uint8_t acc[20];                                                                                                                                                                                                                                  \
        uint64_t drops_amount = (drops_amount_raw);                                                                                                                                                                                                       \
        uint32_t dest_tag = (dest_tag_raw);                                                                                                                                                                                                               \
        uint32_t src_tag = (src_tag_raw);                                                                                                                                                                                                                 \
        uint32_t cls = (uint32_t)ledger_seq();                                                                                                                                                                                                            \
        hook_account(SBUF(acc));                                                                                                                                                                                                                          \
        _01_02_ENCODE_TT(buf_out, ttPAYMENT);              /* uint16  | size   3 */                                                                                                                                                                       \
        _02_02_ENCODE_FLAGS(buf_out, tfCANONICAL);         /* uint32  | size   5 */                                                                                                                                                                       \
        _02_03_ENCODE_TAG_SRC(buf_out, src_tag);           /* uint32  | size   5 */                                                                                                                                                                       \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                /* uint32  | size   5 */                                                                                                                                                                       \
        _02_14_ENCODE_TAG_DST(buf_out, dest_tag);          /* uint32  | size   5 */                                                                                                                                                                       \
        _02_26_ENCODE_FLS(buf_out, cls + 1);               /* uint32  | size   6 */                                                                                                                                                                       \
        _02_27_ENCODE_LLS(buf_out, cls + 5);               /* uint32  | size   6 */                                                                                                                                                                       \
        _06_01_ENCODE_DROPS_AMOUNT(buf_out, drops_amount); /* amount  | size   9 */                                                                                                                                                                       \
        uint8_t *fee_ptr = buf_out;                                                                                                                                                                                                                       \
        _06_08_ENCODE_DROPS_FEE(buf_out, 0);            /* amount  | size   9 */                                                                                                                                                                          \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);     /* pk      | size  35 */                                                                                                                                                                          \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);        /* account | size  22 */                                                                                                                                                                          \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address); /* account | size  22 */                                                                                                                                                                          \
        _0F_09_ENCODE_MEMOS_DUO_GUARDM(buf_out, type1, type1_len, format1, format1_len, data1, data1_len, type2, type2_len, format2, format2_len, data2, data2_len, 1, m);                                                                                \
        int64_t edlen = etxn_details((uint32_t)buf_out, PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_SIZE(type_len, format_len, data_len)); /* emitdet | size 1?? */                                                                                                  \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_PAYMENT_SIMPLE_MEMOS_DUO_SIZE(type_len, format_len, data_len));                                                                                                                               \
        \ 
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                                                                                                                                                                                            \
    }

/////////// Macros to prepare a check with memos. ///////////

#define PREPARE_SIMPLE_CHECK_MEMOS_SINGLE_SIZE(type_len, format_len, data_len) \
    ((type_len + (type_len <= 192 ? 2 : 3) + format_len + (format_len <= 192 ? 2 : 3) + data_len + (data_len <= 192 ? 2 : 3)) + PREPARE_SIMPLE_CHECK_SIZE + 4)
#define PREPARE_SIMPLE_CHECK_MEMOS_SINGLE_GUARDM(buf_out_master, tlamt, drops_fee_raw, to_address, type, type_len, format, format_len, data, data_len, n, m) \
    {                                                                                                                                                        \
        uint8_t *buf_out = buf_out_master;                                                                                                                   \
        uint8_t acc[20];                                                                                                                                     \
        uint64_t drops_fee = (drops_fee_raw);                                                                                                                \
        uint32_t cls = (uint32_t)ledger_seq();                                                                                                               \
        hook_account(SBUF(acc));                                                                                                                             \
        _01_02_ENCODE_TT(buf_out, ttCHECK_CREATE);             /* uint16  | size   3 */                                                                      \
        _02_04_ENCODE_SEQUENCE(buf_out, 0);                    /* uint32  | size   5 */                                                                      \
        _02_26_ENCODE_FLS(buf_out, cls + 1);                   /* uint32  | size   6 */                                                                      \
        _02_27_ENCODE_LLS(buf_out, cls + 5);                   /* uint32  | size   6 */                                                                      \
        _06_09_ENCODE_TL_SENDMAX_GUARDM(buf_out, tlamt, 1, 1); /* amount  | size  48 */                                                                      \
        uint8_t *fee_ptr = buf_out;                                                                                                                          \
        _06_08_ENCODE_DROPS_FEE(buf_out, drops_fee);    /* amount  | size   9 */                                                                             \
        _07_03_ENCODE_SIGNING_PUBKEY_NULL(buf_out);     /* pk      | size  35 */                                                                             \
        _08_01_ENCODE_ACCOUNT_SRC(buf_out, acc);        /* account | size  22 */                                                                             \
        _08_03_ENCODE_ACCOUNT_DST(buf_out, to_address); /* account | size  22 */                                                                             \
        _0F_09_ENCODE_MEMOS_SINGLE_GUARDM(buf_out, type, type_len, format, format_len, data, data_len, n, m + 1);                                            \
        etxn_details((uint32_t)buf_out, PREPARE_SIMPLE_CHECK_MEMOS_SINGLE_SIZE(type_len, format_len, data_len)); /* emitdet | size 1?? */                    \
        int64_t fee = etxn_fee_base(buf_out_master, PREPARE_SIMPLE_CHECK_MEMOS_SINGLE_SIZE(type_len, format_len, data_len));                                 \
        \ 
        _06_08_ENCODE_DROPS_FEE(fee_ptr, fee);                                                                                                               \
    }

/**************************************************************************/
/****************************Utility macros********************************/
/**************************************************************************/

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

#endif
