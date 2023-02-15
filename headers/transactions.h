#ifndef TRANSACTIONS_INCLUDED
#define TRANSACTIONS_INCLUDED 1

/**************************************************************************/
/*************Pre-populated templates of Payment Transactions**************/
/**************************************************************************/

// Simple XRP Payment with single memo.
uint8_t HOOK_UPDATE_RES_PAYMENT[331] = {
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
    0x7C, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MemoType (16 bytes) offset 136
    0x7D, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MemoData (32 bytes) offset 154
    0x7E, 0x03,
    0x00, 0x00, 0x00, // MemoFormat (3 bytes) offset 188
    0xE1, 0xF1,       // Memo array and object end markers
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit_details(138) - Added on prepare to offset 193
    // emit_details - NOTE : Considered additional 22 bytes for the callback scenario.
};

#define HOOK_UPDATE_RES_PAYMENT_TX_SIZE \
    sizeof(HOOK_UPDATE_RES_PAYMENT)
#define PREPARE_HOOK_UPDATE_RES_PAYMENT_TX(drops_amount, to_address, unique_id)                                 \
    {                                                                                                           \
        uint8_t *buf_out = HOOK_UPDATE_RES_PAYMENT;                                                             \
        UINT32_TO_BUF((buf_out + 25), cur_ledger_seq + 1);                                                      \
        UINT32_TO_BUF((buf_out + 31), cur_ledger_seq + 5);                                                      \
        uint8_t *buf_ptr = (buf_out + 35);                                                                      \
        _06_01_ENCODE_DROPS_AMOUNT(buf_ptr, drops_amount);                                                      \
        COPY_20BYTES((buf_out + 90), hook_accid);                                                               \
        COPY_20BYTES((buf_out + 112), to_address);                                                              \
        COPY_16BYTES((buf_out + 136), HOOK_UPDATE_RES);                                                         \
        COPY_32BYTES((buf_out + 154), unique_id);                                                               \
        COPY_2BYTES((buf_out + 188), FORMAT_HEX);                                                               \
        COPY_BYTE((buf_out + 188 + 2), (FORMAT_HEX + 2));                                                       \
        etxn_details((buf_out + 193), HOOK_UPDATE_RES_PAYMENT_TX_SIZE);                                         \
        int64_t fee = etxn_fee_base(buf_out, HOOK_UPDATE_RES_PAYMENT_TX_SIZE);                                  \
        buf_ptr = buf_out + 44;                                                                                 \
        _06_08_ENCODE_DROPS_FEE(buf_ptr, fee); /** Skip the fee check since this tx is sent to governor hook.*/ \
    }

/**************************************************************************/
/*************Pre-populated templates of SetHook Transactions**************/
/**************************************************************************/

// IOU Payment with single memo (Reward).
// Set Hook
uint8_t SET_HOOK_TRANSACTION[705] = {
    0x12, 0x00, 0x16,                                     // transaction_type(ttHOOK_SET)
    0x22, 0x00, 0x00, 0x00, 0x02,                         // flags (tfOnlyXRP)
    0x24, 0x00, 0x00, 0x00, 0x00,                         // sequence(0)
    0x20, 0x1A, 0x00, 0x00, 0x00, 0x00,                   // first_ledger_sequence(ledger_seq + 1) - Added on prepare to offset 15
    0x20, 0x1B, 0x00, 0x00, 0x00, 0x00,                   // last_ledger_sequence(ledger_seq + 5) - Added on prepare to offset 21
    0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // fee - Added on prepare to offset 25
    0x73, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // signing pubkey(NULL)
    0x81, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account - Added on prepare to offset 71
    0xF9, 0xEA, // Memo array and object start markers
    0x7C, 0x0A,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MemoType (10 bytes) offset 95
    0x7D, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MemoData (32 bytes) offset 107
    0x7E, 0x03,
    0x00, 0x00, 0x00, // MemoFormat (3 bytes) offset 141
    0xE1, 0xF1,       // Memo array and object end markers
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // hook objects - Added on prepare to offset 146
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit details(138)
    // hook objects - Added on prepare to offset 146
};

#define ENCODE_HASH_SIZE 34
#define HOOK_HASH 0x1FU
#define HOOK_NAMESPACE 0x20U
#define ENCODE_HASH(buf_out, hash_ptr, t)                           \
    {                                                               \
        buf_out[0] = 0x50U + 0U;                                    \
        buf_out[1] = t;                                             \
        *(uint64_t *)(buf_out + 2) = *(uint64_t *)(hash_ptr + 0);   \
        *(uint64_t *)(buf_out + 10) = *(uint64_t *)(hash_ptr + 8);  \
        *(uint64_t *)(buf_out + 18) = *(uint64_t *)(hash_ptr + 16); \
        *(uint64_t *)(buf_out + 26) = *(uint64_t *)(hash_ptr + 24); \
        buf_out += ENCODE_HASH_SIZE;                                \
    }

#define _05_31_ENCODE_HOOK_HASH(buf_out, hash_ptr) \
    ENCODE_HASH(buf_out, hash_ptr, HOOK_HASH);

#define _05_32_ENCODE_HOOK_NAMESPACE(buf_out, hash_ptr) \
    ENCODE_HASH(buf_out, hash_ptr, HOOK_NAMESPACE);

#define ENCODE_DELETEHOOK_SIZE 2
#define ENCODE_DELETEHOOK(buf_out)         \
    {                                      \
        buf_out[0] = 0x70U + 0xBU;         \
        buf_out[1] = 0x00U;                \
        buf_out += ENCODE_DELETEHOOK_SIZE; \
    }

#define _07_11_ENCODE_DELETE_HOOK(buf_out) \
    ENCODE_DELETEHOOK(buf_out);

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

#define ENCODE_SUB_FIELDS_SIZE 2U
#define ENCODE_SUB_FIELDS(buf_out, type, field) \
    {                                           \
        buf_out[0] = type;                      \
        buf_out[1] = field;                     \
        buf_out += ENCODE_SUB_FIELDS_SIZE;      \
    }

#define ENCODE_GRANT(buf_out, hash, account)                                                   \
    {                                                                                          \
        ENCODE_SUB_FIELDS(buf_out, OBJECT, HOOK_GRANT); /*Obj start*/ /* uint32  | size   2 */ \
        _05_31_ENCODE_HOOK_HASH(buf_out, hash);                       /* uint256 | size  34 */ \
        _08_XX_ENCODE_ACCOUNT(buf_out, account, AUTHORIZE);           /* accid | size 22 */    \
        ENCODE_FIELDS(buf_out, OBJECT, END); /*Object End*/           /* uint32  | size   1 */ \
    }

#define ENCODE_BLOB(buf_out, data, data_len, type) \
    {                                              \
        buf_out[0] = 0x70;                         \
        buf_out[1] = type;                         \
        buf_out[2] = data_len;                     \
        if (data_len == 32)                        \
        {                                          \
            COPY_32BYTES((buf_out + 3), data);     \
        }                                          \
        else if (data_len == 20)                   \
        {                                          \
            COPY_20BYTES((buf_out + 3), data);     \
        }                                          \
        buf_out += (3 + data_len);                 \
    }

#define ENCODE_PARAM(buf_out, key, key_len, value, value_len)                                  \
    {                                                                                          \
        ENCODE_SUB_FIELDS(buf_out, OBJECT, HOOK_PARAM); /*Obj start*/ /* uint32  | size   2 */ \
        ENCODE_BLOB(buf_out, key, key_len, HOOK_PARAM_NAME);                                   \
        ENCODE_BLOB(buf_out, value, value_len, HOOK_PARAM_VALUE);                              \
        ENCODE_FIELDS(buf_out, OBJECT, END); /*Object End*/ /* uint32  | size   1 */           \
    }

#define ENCODE_HOOK_OBJECT(buf_out, hash, namespace, grant_hash1, grant_account1, grant_hash2, grant_account2, param_key, param_key_len, param_value, param_value_len, is_governor) \
    {                                                                                                                                                                               \
        ENCODE_FIELDS(buf_out, OBJECT, HOOK); /*Obj start*/ /* uint32  | size   1 */                                                                                                \
        _02_02_ENCODE_FLAGS(buf_out, tfHookOveride);        /* uint32  | size   5 */                                                                                                \
        if (!IS_BUFFER_EMPTY_32(hash))                                                                                                                                              \
        {                                                                                                                                                                           \
            _05_31_ENCODE_HOOK_HASH(buf_out, hash);           /* uint256 | size  34 */                                                                                              \
            _05_32_ENCODE_HOOK_NAMESPACE(buf_out, namespace); /* uint256 | size  34 */                                                                                              \
            if (is_governor)                                                                                                                                                        \
            {                                                                                                                                                                       \
                ENCODE_SUB_FIELDS(buf_out, ARRAY, HOOK_GRANTS); /*Array start*/ /* uint32  | size   2 */                                                                            \
                ENCODE_GRANT(buf_out, grant_hash1, grant_account1);                                                                                                                 \
                ENCODE_GRANT(buf_out, grant_hash2, grant_account2);                                                                                                                 \
                ENCODE_FIELDS(buf_out, ARRAY, END); /*Array end*/ /* uint32  | size   1 */                                                                                          \
            }                                                                                                                                                                       \
            else                                                                                                                                                                    \
            {                                                                                                                                                                       \
                ENCODE_SUB_FIELDS(buf_out, ARRAY, HOOK_PARAMS); /*Array start*/ /* uint32  | size   2 */                                                                            \
                ENCODE_PARAM(buf_out, param_key, param_key_len, param_value, param_value_len);                                                                                      \
                ENCODE_FIELDS(buf_out, ARRAY, END); /*Array end*/ /* uint32  | size   1 */                                                                                          \
            }                                                                                                                                                                       \
        }                                                                                                                                                                           \
        else                                                                                                                                                                        \
        {                                                                                                                                                                           \
            _07_11_ENCODE_DELETE_HOOK(buf_out); /* blob    | size   2 */                                                                                                            \
        }                                                                                                                                                                           \
        ENCODE_FIELDS(buf_out, OBJECT, END); /*Arr End*/ /* uint32  | size   1 */                                                                                                   \
    }

#define PREPARE_SET_HOOK_WITH_GRANT_TRANSACTION_TX(hash_arr, namespace, unique_id, grant_account1, grant_hash1, grant_account2, grant_hash2, param_key, param_key_len, param_value, param_value_len, is_governor, tx_size) \
    {                                                                                                                                                                                                                      \
        uint8_t *buf_out = SET_HOOK_TRANSACTION;                                                                                                                                                                           \
        UINT32_TO_BUF((buf_out + 15), cur_ledger_seq + 1);                                                                                                                                                                 \
        UINT32_TO_BUF((buf_out + 21), cur_ledger_seq + 5);                                                                                                                                                                 \
        COPY_20BYTES((buf_out + 71), hook_accid);                                                                                                                                                                          \
        COPY_10BYTES((buf_out + 95), SET_HOOK);                                                                                                                                                                            \
        COPY_32BYTES((buf_out + 107), unique_id);                                                                                                                                                                          \
        COPY_2BYTES((buf_out + 141), FORMAT_HEX);                                                                                                                                                                          \
        COPY_BYTE((buf_out + 141 + 2), (FORMAT_HEX + 2));                                                                                                                                                                  \
        uint8_t *cur_ptr = buf_out + 146;                                                                                                                                                                                  \
        ENCODE_FIELDS(cur_ptr, ARRAY, HOOKS);                                                                                                                                                                              \
        ENCODE_HOOK_OBJECT(cur_ptr, hash_arr, namespace, grant_hash1, grant_account1, grant_hash2, grant_account2, param_key, param_key_len, param_value, param_value_len, is_governor);                                   \
        ENCODE_HOOK_OBJECT(cur_ptr, (hash_arr + HASH_SIZE), namespace, 0, 0, 0, 0, 0, 0, 0, 0, 0);                                                                                                                         \
        ENCODE_HOOK_OBJECT(cur_ptr, (hash_arr + (2 * HASH_SIZE)), namespace, 0, 0, 0, 0, 0, 0, 0, 0, 0);                                                                                                                   \
        ENCODE_HOOK_OBJECT(cur_ptr, (hash_arr + (3 * HASH_SIZE)), namespace, 0, 0, 0, 0, 0, 0, 0, 0, 0);                                                                                                                   \
        ENCODE_FIELDS(cur_ptr, ARRAY, END);                                                                                                                                                                                \
        tx_size = cur_ptr - buf_out + 138;                                                                                                                                                                                 \
        etxn_details(cur_ptr, 138);                                                                                                                                                                                        \
        int64_t fee = etxn_fee_base(buf_out, tx_size);                                                                                                                                                                     \
        cur_ptr = buf_out + 25;                                                                                                                                                                                            \
        _06_08_ENCODE_DROPS_FEE(cur_ptr, fee); /** Skip the fee check since this tx is a sethook **/                                                                                                                       \
        TRACEHEX(SET_HOOK_TRANSACTION);                                                                                                                                                                                    \
    }

#define PREPARE_SET_HOOK_TRANSACTION_TX(hash_arr, namespace, unique_id, param_key, param_key_len, param_value, param_value_len, tx_size) \
    PREPARE_SET_HOOK_WITH_GRANT_TRANSACTION_TX(hash_arr, namespace, unique_id, 0, 0, 0, 0, param_key, param_key_len, param_value, param_value_len, 0, tx_size);

#endif
