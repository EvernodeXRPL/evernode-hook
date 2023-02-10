#ifndef TRANSACTIONS_INCLUDED
#define TRANSACTIONS_INCLUDED 1

/**************************************************************************/
/*************Pre-populated templates of Payment Transactions**************/
/**************************************************************************/

// IOU Payment with single memo (Reward).
// Set Hook
uint8_t SET_HOOK_TRANSACTION[465] = {
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
    0x00, 0x00, // hook objects - Added on prepare to offset 91
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit details(138)
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

#define ENCODE_HOOK_OBJECT(buf_out, hash, namespace)                                   \
    {                                                                                  \
        ENCODE_FIELDS(buf_out, OBJECT, HOOK); /*Obj start*/ /* uint32  | size   1 */   \
        _02_02_ENCODE_FLAGS(buf_out, tfHookOveride);        /* uint32  | size   5 */   \
        if (!IS_BUFFER_EMPTY_32(hash))                                                 \
        {                                                                              \
            _05_31_ENCODE_HOOK_HASH(buf_out, hash);           /* uint256 | size  34 */ \
            _05_32_ENCODE_HOOK_NAMESPACE(buf_out, namespace); /* uint256 | size  34 */ \
        }                                                                              \
        else                                                                           \
        {                                                                              \
            _07_11_ENCODE_DELETE_HOOK(buf_out); /* blob    | size   2 */               \
        }                                                                              \
        ENCODE_FIELDS(buf_out, OBJECT, END); /*Arr End*/ /* uint32  | size   1 */      \
    }

#define PREPARE_SET_HOOK_TRANSACTION_TX(hash_arr, namespace, tx_size)         \
    {                                                                         \
        uint8_t *buf_out = SET_HOOK_TRANSACTION;                              \
        UINT32_TO_BUF((buf_out + 15), cur_ledger_seq + 1);                    \
        UINT32_TO_BUF((buf_out + 21), cur_ledger_seq + 5);                    \
        COPY_20BYTES((buf_out + 71), hook_accid);                             \
        uint8_t *cur_ptr = buf_out + 91;                                      \
        ENCODE_FIELDS(cur_ptr, ARRAY, HOOKS);                                 \
        ENCODE_HOOK_OBJECT(cur_ptr, hash_arr, namespace);                     \
        ENCODE_HOOK_OBJECT(cur_ptr, (hash_arr + HASH_SIZE), namespace);       \
        ENCODE_HOOK_OBJECT(cur_ptr, (hash_arr + (2 * HASH_SIZE)), namespace); \
        ENCODE_HOOK_OBJECT(cur_ptr, (hash_arr + (3 * HASH_SIZE)), namespace); \
        ENCODE_FIELDS(cur_ptr, ARRAY, END);                                   \
        tx_size = cur_ptr - buf_out + 138;                                    \
        etxn_details(cur_ptr, 138);                                           \
        int64_t fee = etxn_fee_base(buf_out, tx_size);                        \
        cur_ptr = SET_HOOK_TRANSACTION + 25;                                  \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(cur_ptr, fee);                         \
    }

#endif
