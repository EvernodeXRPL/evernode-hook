#include "../lib/hookapi.h"

// Executed when an emitted transaction is successfully accepted into a ledger
// or when an emitted transaction cannot be accepted into any ledger (with what = 1),
int64_t cbak(uint32_t reserved)
{
    return 0;
}

// Executed whenever a transaction comes into or leaves from the account the Hook is set on.
int64_t hook(uint32_t reserved)
{
    int64_t txn_type = otxn_type();
    if (txn_type == ttPAYMENT)
    {

        uint8_t account_field[20];
        int32_t account_field_len = otxn_field(SBUF(account_field), sfAccount);

        uint8_t hook_accid[20];
        if (hook_account(SBUF(hook_accid)) < 0)
            rollback(SBUF("Doubler: Could not fetch hook account id."), 1);
        int equal = 0;
        BUFFER_EQUAL(equal, account_field, hook_accid, 20);
        if (equal)
            accept(SBUF("Outgoing accept accept."), 0);

        uint8_t memos[4096];
        int64_t memos_len = otxn_field(SBUF(memos), sfMemos);

        if (memos_len)
        {
            uint8_t *memo_ptr, *type_ptr, *format_ptr, *data_ptr;
            uint32_t memo_len, type_len, format_len, data_len;
            int64_t memo_lookup = sto_subarray(memos, memos_len, 0);

            memo_ptr = SUB_OFFSET(memo_lookup) + memos;
            memo_len = SUB_LENGTH(memo_lookup);

            memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemo);
            memo_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;
            memo_len = SUB_LENGTH(memo_lookup);
            if (memo_lookup < 0)
                accept(SBUF("Evernode: Incoming txn had a blank sfMemos."), 1);
            memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemoType);
            type_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;
            type_len = SUB_LENGTH(memo_lookup);
            memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemoFormat);
            format_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;
            format_len = SUB_LENGTH(memo_lookup);
            memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemoData);
            data_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;
            data_len = SUB_LENGTH(memo_lookup);

            int is_state = 0;
            BUFFER_EQUAL_STR_GUARD(is_state, type_ptr, type_len, "state", 5);
            if (is_state)
            {
                if (state_set(((data_len - 32 > 0) ? (data_ptr + 32) : 0), data_len - 32, data_ptr, 32) < 0)
                    rollback(SBUF("Test: Failed to add state."), 3);
                accept(SBUF("Successfully migrated."), 0);
            }
        }
    }

    accept(SBUF("Success"), 0);
    _g(0, 0);
    return 0;
}
