#include "governor.h"

// Executed when an emitted transaction is successfully accepted into a ledger
// or when an emitted transaction cannot be accepted into any ledger (with what = 1),
int64_t cbak(uint32_t reserved)
{
    return 0;
}

// Executed whenever a transaction comes into or leaves from the account the Hook is set on.
int64_t hook(uint32_t reserved)
{
    uint8_t meta_params[META_PARAMS_SIZE];
    if (hook_param(SBUF(meta_params), SBUF(META_PARAMS)) < 0 || meta_params[CHAIN_IDX_PARAM_OFFSET] != 1)
        rollback(SBUF("Evernode: Invalid meta params for chain one."), 1);

    uint8_t op_type = meta_params[OP_TYPE_PARAM_OFFSET];
    uint8_t *seq_param_ptr = &meta_params[CUR_LEDGER_SEQ_PARAM_OFFSET];
    int64_t cur_ledger_seq = INT64_FROM_BUF(seq_param_ptr);
    uint8_t *ts_param_ptr = &meta_params[CUR_LEDGER_TIMESTAMP_PARAM_OFFSET];
    int64_t cur_ledger_timestamp = INT64_FROM_BUF(ts_param_ptr);
    unsigned char hook_accid[ACCOUNT_ID_SIZE];
    COPY_20BYTES(hook_accid, (meta_params + HOOK_ACCID_PARAM_OFFSET));
    uint8_t account_field[ACCOUNT_ID_SIZE];
    COPY_20BYTES(account_field, (meta_params + ACCOUNT_FIELD_PARAM_OFFSET));
    uint8_t *issuer_accid = &meta_params[ISSUER_PARAM_OFFSET];
    uint8_t *foundation_accid = &meta_params[FOUNDATION_PARAM_OFFSET];

    // Memos can be empty for some transactions.
    uint8_t memo_params[MEMO_PARAM_SIZE];
    const res = hook_param(SBUF(memo_params), SBUF(MEMO_PARAMS));
    if (res != DOESNT_EXIST && res < 0)
        rollback(SBUF("Evernode: Could not get memo params for chain one."), 1);

    uint8_t chain_one_params[CHAIN_ONE_PARAMS_SIZE];
    if (hook_param(SBUF(chain_one_params), SBUF(CHAIN_ONE_PARAMS)) < 0)
        rollback(SBUF("Evernode: Could not get params for chain one."), 1);
    uint8_t *amount_buffer = &chain_one_params[AMOUNT_BUF_PARAM_OFFSET];
    uint8_t *float_amt_ptr = &chain_one_params[FLOAT_AMT_PARAM_OFFSET];
    int64_t float_amt = INT64_FROM_BUF(float_amt_ptr);
    uint8_t *txid = &chain_one_params[TXID_PARAM_OFFSET];

    if (op_type == OP_SET_HOOK)
    {
        uint8_t initializer_accid[ACCOUNT_ID_SIZE];
        const int initializer_accid_len = util_accid(SBUF(initializer_accid), HOOK_INITIALIZER_ADDR, 35);
        if (initializer_accid_len < ACCOUNT_ID_SIZE)
            rollback(SBUF("Evernode: Could not convert initializer account id."), 1);

        if (!BUFFER_EQUAL_20(initializer_accid, account_field))
            rollback(SBUF("Evernode: Only initializer is allowed to trigger a hook set."), 1);

        etxn_reserve(1);
        uint32_t txn_size;
        PREPARE_SET_HOOK_TX(memo_params, NAMESPACE, txn_size);

        uint8_t emithash[HASH_SIZE];
        if (emit(SBUF(emithash), SET_HOOK, txn_size) < 0)
            rollback(SBUF("Evernode: Hook set transaction failed."), 1);
        trace(SBUF("emit hash: "), SBUF(emithash), 1);

        accept(SBUF("Evernode: Successfully emitted SetHook transaction."), 0);
    }

    accept(SBUF("Evernode: Transaction is not handled in Hook Position 1."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
