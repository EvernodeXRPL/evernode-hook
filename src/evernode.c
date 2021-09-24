#include "../lib/hookapi.h"
#include "evernode.h"

#define MAX_MEMO_SIZE 4096 // Maximum tx blob size.

// Executed when an emitted transaction is successfully accepted into a ledger
// or when an emitted transaction cannot be accepted into any ledger (with what = 1),
int64_t cbak(int64_t reserved)
{
    return 0;
}

// Executed whenever a transaction comes into or leaves from the account the Hook is set on.
int64_t hook(int64_t reserved)
{
    // Getting the hook account id.
    unsigned char hook_accid[20];
    hook_account((uint32_t)hook_accid, 20);

    // Next fetch the sfAccount field from the originating transaction
    uint8_t account_field[20];
    int32_t account_field_len = otxn_field(SBUF(account_field), sfAccount);
    if (account_field_len < 20)
        rollback(SBUF("Evernode: sfAccount field is missing."), 1);

    // Accept any outgoing transactions without further processing.
    int is_outgoing = 0;
    BUFFER_EQUAL(is_outgoing, hook_accid, account_field, 20);
    if (is_outgoing)
        accept(SBUF("Evernode: Outgoing transaction. Passing."), 0);

    // ****************************************************************************************************
    // Setting and loading configuration values from the hook state.
    // Currently these values aren't used.
    // So, this need to be moved to the needed section when the usage is implemented.
    uint64_t conf_mint_limit;
    GET_CONF_VALUE(conf_mint_limit, DEF_MINT_LIMIT, CONF_MINT_LIMIT, "Evernode: Could not set default state for mint limit.");
    TRACEVAR(conf_mint_limit);
    // ****************************************************************************************************

    int64_t txn_type = otxn_type();
    if (txn_type == ttPAYMENT)
    {
        // specifically we're interested in the amount sent
        int64_t oslot = otxn_slot(0);
        if (oslot < 0)
            rollback(SBUF("Evernode: Could not slot originating txn."), 1);

        int64_t amt_slot = slot_subfield(oslot, sfAmount, 0);

        if (amt_slot < 0)
            rollback(SBUF("Evernode: Could not slot otxn.sfAmount"), 1);

        int64_t is_xrp = slot_type(amt_slot, 1);
        if (is_xrp < 0)
            rollback(SBUF("Evernode: Could not determine sent amount type"), 1);

        // Memos
        uint8_t memos[MAX_MEMO_SIZE];
        int64_t memos_len = otxn_field(SBUF(memos), sfMemos);

        if (!memos_len)
            accept(SBUF("Evernode: No memos found."), 1);

        // since our memos are in a buffer inside the hook (as opposed to being a slot) we use the sto api with it
        // the sto apis probe into a serialized object returning offsets and lengths of subfields or array entries
        int64_t memo_lookup = sto_subarray(memos, memos_len, 0);

        uint8_t *memo_ptr = SUB_OFFSET(memo_lookup) + memos;
        uint32_t memo_len = SUB_LENGTH(memo_lookup);

        // memos are nested inside an actual memo object, so we need to subfield
        // equivalently in JSON this would look like memo_array[i]["Memo"]
        memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemo);
        memo_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;
        memo_len = SUB_LENGTH(memo_lookup);

        if (memo_lookup < 0)
            accept(SBUF("Evernode: Incoming txn had a blank sfMemos."), 1);

        int64_t type_lookup = sto_subfield(memo_ptr, memo_len, sfMemoType);
        uint8_t *type_ptr = SUB_OFFSET(type_lookup) + memo_ptr;
        uint32_t type_len = SUB_LENGTH(type_lookup);
        // trace(SBUF("type in hex: "), type_ptr, type_len, 1);

        int64_t format_lookup = sto_subfield(memo_ptr, memo_len, sfMemoFormat);
        uint8_t *format_ptr = SUB_OFFSET(format_lookup) + memo_ptr;
        uint32_t format_len = SUB_LENGTH(format_lookup);
        // trace(SBUF("format in hex: "), format_ptr, format_len, 1);

        int64_t data_lookup = sto_subfield(memo_ptr, memo_len, sfMemoData);
        uint8_t *data_ptr = SUB_OFFSET(data_lookup) + memo_ptr;
        uint32_t data_len = SUB_LENGTH(data_lookup);
        // trace(SBUF("data in hex: "), data_ptr, data_len, 1); // Text data is in hex format.

        if (is_xrp)
        {
            int is_redeem_ref = 0;
            BUFFER_EQUAL_STR_GUARD(is_redeem_ref, type_ptr, type_len, REDEEM_REF, 1);
            if (is_redeem_ref)
            {
                int is_format_match = 0;
                BUFFER_EQUAL_STR_GUARD(is_format_match, format_ptr, format_len, FORMAT_BINARY, 1);
                if (!is_format_match)
                    rollback(SBUF("Evernode: Redeem reference memo format should be binary."), 50);

                if (data_len != 64)
                    rollback(SBUF("Evernode: Invalid redeem reference."), 1);

                uint8_t hash_ptr[HASH_SIZE];
                HEXSTR_TO_BYTES(hash_ptr, data_ptr, data_len);

                // Redeem response has 2 memos, so check for the type in second memo.
                memo_lookup = sto_subarray(memos, memos_len, 1);

                memo_ptr = SUB_OFFSET(memo_lookup) + memos;
                memo_len = SUB_LENGTH(memo_lookup);

                memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemo);
                memo_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;
                memo_len = SUB_LENGTH(memo_lookup);

                if (memo_lookup < 0)
                    accept(SBUF("Evernode: Incoming redeem reference txn had a blank sfMemos."), 1);

                type_lookup = sto_subfield(memo_ptr, memo_len, sfMemoType);
                type_ptr = SUB_OFFSET(type_lookup) + memo_ptr;
                type_len = SUB_LENGTH(type_lookup);
                // trace(SBUF("type in hex: "), type_ptr, type_len, 1);

                format_lookup = sto_subfield(memo_ptr, memo_len, sfMemoFormat);
                format_ptr = SUB_OFFSET(format_lookup) + memo_ptr;
                format_len = SUB_LENGTH(format_lookup);
                // trace(SBUF("format in hex: "), format_ptr, format_len, 1);

                data_lookup = sto_subfield(memo_ptr, memo_len, sfMemoData);
                data_ptr = SUB_OFFSET(data_lookup) + memo_ptr;
                data_len = SUB_LENGTH(data_lookup);
                // trace(SBUF("data in hex: "), data_ptr, data_len, 1); // Text data is in hex format.

                // Redeem response should contain redeemResp and format should be binary.
                int is_redeem_res = 0;
                BUFFER_EQUAL_STR_GUARD(is_redeem_res, type_ptr, type_len, REDEEM_RESP, 1);
                if (!is_redeem_res)
                    rollback(SBUF("Evernode: Redeem response does not have instance info."), 1);

                is_format_match = 0;
                BUFFER_EQUAL_STR_GUARD(is_format_match, format_ptr, format_len, FORMAT_BINARY, 1);
                if (!is_format_match)
                    rollback(SBUF("Evernode: Redeem response memo format should be binary."), 50);

                // Check for state with key as redeemRef.
                REDEEM_OP_KEY(hash_ptr);

                uint8_t redeem_op[REDEEM_STATE_VAL_SIZE];
                if (state(SBUF(redeem_op), SBUF(STP_REDEEM_OP)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: No redeem state for the redeem response."), 1);

                int is_error = 0;
                BUFFER_EQUAL_STR_GUARD(is_error, data_ptr, data_len, REDEEM_ERR, 1);
                // Send hosting tokens to the host and clear the state only if there's no error.
                if (!is_error)
                {
                    // Reserving one transaction.
                    etxn_reserve(1);

                    int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                    // Prepare currency.
                    uint8_t host_currency[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, redeem_op[0], redeem_op[1], redeem_op[2], 0, 0, 0, 0, 0};
                    uint8_t *host_amount_ptr = &redeem_op[3];
                    int64_t host_amount = INT64_FROM_BUF(host_amount_ptr);
                    uint8_t *issuer_ptr = &redeem_op[11];

                    // We need to dump the iou amount into a buffer.
                    // by supplying -1 as the fieldcode we tell float_sto not to prefix an actual STO header on the field.
                    uint8_t amt_out[AMOUNT_BUF_SIZE];
                    if (float_sto(SBUF(amt_out), SBUF(host_currency), issuer_ptr, 20, host_amount, -1) < 0)
                        rollback(SBUF("Evernode: Could not dump hosting token amount into sto"), 1);

                    // Set the currency code and issuer in the amount field
                    for (int i = 0; GUARD(20), i < 20; ++i)
                    {
                        amt_out[i + 28] = issuer_ptr[i];
                        amt_out[i + 8] = host_currency[i];
                    }

                    // Create the outgoing hosting token txn.
                    uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                    PREPARE_PAYMENT_SIMPLE_TRUSTLINE(txn_out, amt_out, fee, account_field, 0, 0);

                    uint8_t emithash[HASH_SIZE];
                    if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting txn failed"), 1);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);

                    if (state_set(0, 0, SBUF(STP_REDEEM_OP)) < 0)
                        rollback(SBUF("Evernode: Could not delete state for redeem_op."), 1);

                    accept(SBUF("Redeem response successful."), 0);
                }
                else
                    // If error refund the host tokens back to user.
                    accept(SBUF("Redeem response failed."), 1);
            }

            // Refund.
            int is_refund_request = 0;
            BUFFER_EQUAL_STR_GUARD(is_refund_request, type_ptr, type_len, REFUND, 1);
            if (is_refund_request)
            {
                int is_format_match = 0;
                BUFFER_EQUAL_STR_GUARD(is_format_match, format_ptr, format_len, FORMAT_BINARY, 1);
                if (!is_format_match)
                    rollback(SBUF("Evernode: Memo format should be binary in refund request."), 1);

                if (data_len != 64) // 64 bytes is the size of the hash in hex
                    rollback(SBUF("Evernode: Memo data should be 64 bytes in hex in refund request."), 1);

                uint8_t tx_hash_bytes[HASH_SIZE];
                HEXSTR_TO_BYTES(tx_hash_bytes, data_ptr, data_len);
                REDEEM_OP_KEY(tx_hash_bytes);

                uint8_t data_arr[REDEEM_STATE_VAL_SIZE];

                if (state(SBUF(data_arr), SBUF(STP_REDEEM_OP)) < 0)
                    rollback(SBUF("Evernode: No redeem for this tx hash."), 1);

                uint8_t hosting_token[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, data_arr[0], data_arr[1], data_arr[2], 0, 0, 0, 0, 0};

                // Take the redeem window from the config.
                uint16_t conf_redeem_window;
                GET_CONF_VALUE(conf_redeem_window, DEF_REDEEM_WINDOW, CONF_REDEEM_WINDOW, "Evernode: Could not set default state for redeem window.");
                TRACEVAR(conf_redeem_window);

                uint8_t *ptr = &data_arr[31];
                int64_t ledger_seq_def = ledger_seq() - INT64_FROM_BUF(ptr);
                if (ledger_seq_def < conf_redeem_window)
                    rollback(SBUF("Evernode: Redeeming window is not yet passed. Rejected."), 1);

                // Setup the outgoing txn.
                // Reserving one transaction.
                etxn_reserve(1);
                int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                // We need to dump the iou amount into a buffer.
                // by supplying -1 as the fieldcode we tell float_sto not to prefix an actual STO header on the field
                uint8_t amt_out[AMOUNT_BUF_SIZE];
                uint8_t *issuer_arr = &data_arr[11];
                uint8_t *amount_ptr = &data_arr[3];
                int64_t token_amount = INT64_FROM_BUF(amount_ptr);
                if (float_sto(SBUF(amt_out), SBUF(hosting_token), issuer_arr, 20, token_amount, -1) < 0)
                    rollback(SBUF("Evernode: Could not dump hosting token amount into sto for refund."), 1);

                // Set the currency code and issuer in the amount field
                for (int i = 0; GUARD(20), i < 20; ++i)
                {
                    amt_out[i + 28] = issuer_arr[i];
                    amt_out[i + 8] = hosting_token[i];
                }

                // Finally create the outgoing txn.
                uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                PREPARE_PAYMENT_SIMPLE_TRUSTLINE(txn_out, amt_out, fee, account_field, 0, 0);

                uint8_t emithash[HASH_SIZE];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting refund transaction failed."), 1);

                trace(SBUF("refund tx hash: "), SBUF(emithash), 1);

                // Delete state entry after refund.
                if (state_set(0, 0, SBUF(STP_REDEEM_OP)) < 0)
                    rollback(SBUF("Evernode: Could not delete state entry after refund."), 1);

                accept(SBUF("Evernode: Refund operation successful."), 0);
            }

            // Audit request.
            int is_audit_request = 0;
            BUFFER_EQUAL_STR_GUARD(is_audit_request, type_ptr, type_len, AUDIT_REQ, 1);

            // Audit success response.
            int is_audit_success = 0;
            BUFFER_EQUAL_STR_GUARD(is_audit_success, type_ptr, type_len, AUDIT_SUCCESS, 1);

            if (is_audit_request || is_audit_success)
            {
                // Common checks for both audit request and audit suceess response.

                // Audit request is only served if at least one host is registered.
                // If host count state does not exist, set host count to 0.
                uint8_t host_count_buf[4] = {0};
                uint32_t host_count = 0;
                if (state(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) != DOESNT_EXIST)
                    host_count = UINT32_FROM_BUF(host_count_buf);
                if (host_count == 0)
                    rollback(SBUF("Evernode: No hosts registered to audit."), 1);

                int is_format_match = 0;
                BUFFER_EQUAL_STR_GUARD(is_format_match, format_ptr, format_len, FORMAT_BINARY, 1);
                if (!is_format_match)
                    rollback(SBUF("Evernode: Memo format should be binary for auditing."), 1);

                // If default auditor is not set, first set the default auditor.
                uint8_t auditor_count_buf[4] = {0};
                if (state(SBUF(auditor_count_buf), SBUF(STK_AUDITOR_COUNT)) == DOESNT_EXIST)
                {
                    // Setting up default auditor if no auditors registered.
                    uint8_t auditor_accid[20];
                    util_accid(SBUF(auditor_accid), SBUF(DEF_AUDITOR_ADDR));
                    uint8_t auditor_id_buf[4];
                    // Id of the default auditor in 1.
                    UINT32_TO_BUF(auditor_id_buf, 1);
                    AUDITOR_ID_KEY(auditor_id_buf);
                    if (state_set(SBUF(auditor_accid), SBUF(STP_AUDITOR_ID)) < 0)
                        rollback(SBUF("Evernode: Could not set state for default auditor_id."), 1);

                    uint8_t auditor_addr_buf[AUDITOR_ADDR_VAL_SIZE];
                    auditor_addr_buf[0] = auditor_id_buf[0];
                    auditor_addr_buf[1] = auditor_id_buf[1];
                    auditor_addr_buf[2] = auditor_id_buf[2];
                    auditor_addr_buf[3] = auditor_id_buf[3];
                    // Set 0's to the rest.
                    for (int i = 4; GUARD(AUDITOR_ADDR_VAL_SIZE - 4), i < AUDITOR_ADDR_VAL_SIZE; ++i)
                        auditor_addr_buf[i] = 0;
                    AUDITOR_ADDR_KEY(auditor_accid);
                    if (state_set(SBUF(auditor_addr_buf), SBUF(STP_AUDITOR_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not set state for default auditor_addr."), 1);

                    // Set auditor count to 1;
                    UINT32_TO_BUF(auditor_count_buf, 1);
                    if (state_set(SBUF(auditor_count_buf), SBUF(STK_AUDITOR_COUNT)) < 0)
                        rollback(SBUF("Evernode: Could not set default state for auditor count."), 1);
                }

                // Checking whether this auditor exists.
                AUDITOR_ADDR_KEY(account_field);
                uint8_t auditor_addr_buf[AUDITOR_ADDR_VAL_SIZE]; // <auditor_id(4)><moment_start_idx(8)><host_addr(20)>
                if (state(SBUF(auditor_addr_buf), SBUF(STP_AUDITOR_ADDR)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: Auditor is not registered."), 1);

                // Get moment data from the config.
                uint64_t moment_base_idx;
                GET_CONF_VALUE(moment_base_idx, 0, STK_MOMENT_BASE_IDX, "Evernode: Could not set default state for moment base idx.");
                TRACEVAR(moment_base_idx);

                uint16_t conf_moment_size;
                GET_CONF_VALUE(conf_moment_size, DEF_MOMENT_SIZE, CONF_MOMENT_SIZE, "Evernode: Could not set default state for moment size.");
                TRACEVAR(conf_moment_size);

                // Take current moment start idx.
                uint64_t relative_n = (ledger_seq() - moment_base_idx) / conf_moment_size;
                uint64_t cur_moment_start_idx = moment_base_idx + (relative_n * conf_moment_size);

                // We do not serve audit requests if moment start index is 0.
                if (cur_moment_start_idx == 0)
                    rollback(SBUF("Evernode: Rewards aren't allowed in the first moment."), 1);

                uint32_t auditor_id = UINT32_FROM_BUF(auditor_addr_buf);
                uint64_t lst_moment_start_idx = UINT64_FROM_BUF(&auditor_addr_buf[4]);
                uint8_t *lst_host_addr_ptr = &auditor_addr_buf[12];

                // Seperate logic for audit request and audit suceess response.
                if (is_audit_request) // Audit request
                {
                    // If auditors assigned moment idx is equal to currect moment start idx.
                    // A host has been already assigned.
                    if (lst_moment_start_idx == cur_moment_start_idx)
                        rollback(SBUF("Evernode: A host is already assigned to audit for this moment."), 1);

                    uint8_t moment_seed_buf[MOMENT_SEED_VAL_SIZE]; // <moment_start_idx(8)><moment_seed(32)>
                    // Set the seed if not exist or last updated seed is expired.
                    if (state(SBUF(moment_seed_buf), SBUF(STK_MOMENT_SEED)) == DOESNT_EXIST || UINT64_FROM_BUF(moment_seed_buf) < cur_moment_start_idx)
                    {
                        UINT64_TO_BUF(moment_seed_buf, cur_moment_start_idx);
                        ledger_last_hash(&moment_seed_buf[8], HASH_SIZE);

                        if (state_set(SBUF(moment_seed_buf), SBUF(STK_MOMENT_SEED)) < 0)
                            rollback(SBUF("Evernode: Could not set state for moment seed."), 1);
                    }

                    uint8_t *moment_seed_ptr = &moment_seed_buf[8];
                    trace(SBUF("moment seed: "), moment_seed_ptr, HASH_SIZE, 1);

                    // Take the max reward from the config.
                    uint16_t conf_max_reward;
                    GET_CONF_VALUE(conf_max_reward, DEF_MAX_REWARD, CONF_MAX_REWARD, "Evernode: Could not set default state for max reward.");
                    TRACEVAR(conf_max_reward);

                    // Calculate the host id using seed.
                    // Selecting a host to audit.
                    /////////////////////////////////// Method 1 //////////////////////////////////////////
                    // Only serve if auditor id is less than max reward.
                    if (auditor_id > conf_max_reward)
                        rollback(SBUF("Evernode: Max number of audits per moment is exceeded."), 1);
                    uint32_t host_id = (UINT32_FROM_BUF(moment_seed_ptr + (auditor_id - 1)) % host_count) + 1;
                    /////////////////////////////////// Method 2 //////////////////////////////////////////
                    // uint32_t host_id = 0;
                    // uint32_t lookup_value = 0;
                    // for (int i = 0; GUARD(conf_max_reward), i < conf_max_reward; ++i)
                    // {
                    //     lookup_value = UINT32_FROM_BUF(moment_seed_ptr + i);
                    //     if (((lookup_value % auditor_count) + 1) == auditor_id)
                    //         host_id = (lookup_value % host_count) + 1;
                    // }
                    // if (host_id == 0)
                    //     rollback(SBUF("Evernode: Could not find a host to audit."), 1);
                    ///////////////////////////////////////////////////////////////////////////////////////

                    // Take the host address.
                    uint8_t host_addr[20];
                    uint8_t host_id_arr[4];
                    UINT32_TO_BUF(host_id_arr, host_id);
                    HOST_ID_KEY(host_id_arr);
                    if (state(SBUF(host_addr), SBUF(STP_HOST_ID)) == DOESNT_EXIST)
                        rollback(SBUF("Evernode: Could not find a matching host for the id."), 1);

                    // Take the last audit assigned moment.
                    HOST_ADDR_KEY(host_addr);
                    uint8_t host_addr_buf[HOST_ADDR_VAL_SIZE]; // <host_id(4)><hosting_token(3)><audit_assigned_moment_start_idx(8)><rewarded_moment_start_idx(8)>
                    if (state(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                        rollback(SBUF("Evernode: Host is not registered."), 1);

                    uint8_t *host_token_ptr = &host_addr_buf[4];

                    // If host is already assigned for audit within this moment we won't reward again.
                    if (UINT64_FROM_BUF(&host_addr_buf[7]) == cur_moment_start_idx)
                        rollback(SBUF("Evernode: Picked host is already assigned for audit within this moment."), 1);

                    trace(SBUF("Hosting token"), host_token_ptr, 3, 1);

                    // Take the minimum redeem amount from the config.
                    uint16_t conf_min_redeem;
                    GET_CONF_VALUE(conf_min_redeem, DEF_MIN_REDEEM, CONF_MIN_REDEEM, "Evernode: Could not set default state for min redeem.");
                    TRACEVAR(conf_min_redeem);

                    // Setup the outgoing txn.
                    // Reserving one transaction.
                    etxn_reserve(1);
                    int64_t fee = etxn_fee_base(PREPARE_SIMPLE_CHECK_SIZE);

                    // We need to dump the iou amount into a buffer.
                    // by supplying -1 as the fieldcode we tell float_sto not to prefix an actual STO header on the field
                    uint8_t hosting_token[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, host_token_ptr[0], host_token_ptr[1], host_token_ptr[2], 0, 0, 0, 0, 0};
                    uint8_t amt_out[AMOUNT_BUF_SIZE];
                    int64_t token_limit = float_set(0, conf_min_redeem);
                    if (float_sto(SBUF(amt_out), SBUF(hosting_token), SBUF(host_addr), token_limit, -1) < 0)
                        rollback(SBUF("Evernode: Could not dump hosting token amount into sto for check."), 1);

                    // Set the currency code and issuer in the amount field
                    for (int i = 0; GUARD(20), i < 20; ++i)
                    {
                        amt_out[i + 28] = host_addr[i];
                        amt_out[i + 8] = hosting_token[i];
                    }

                    // Finally create the outgoing txn.
                    uint8_t txn_out[PREPARE_SIMPLE_CHECK_SIZE];
                    PREPARE_SIMPLE_CHECK(txn_out, amt_out, fee, account_field);

                    uint8_t emithash[HASH_SIZE];
                    if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting hosting token check failed."), 1);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);

                    // Update the auditor state.
                    for (int i = 0; GUARD(8), i < 8; ++i)
                        auditor_addr_buf[i + 4] = moment_seed_buf[i];
                    for (int i = 0; GUARD(20), i < 20; ++i)
                        auditor_addr_buf[i + 12] = host_addr[i];
                    if (state_set(SBUF(auditor_addr_buf), SBUF(STP_AUDITOR_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not update state for auditor_addr."), 1);

                    // Update the host's audit state.
                    for (int i = 0; GUARD(8), i < 8; ++i)
                        host_addr_buf[i + 7] = moment_seed_buf[i];
                    if (state_set(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not update audit moment for host_addr."), 1);

                    accept(SBUF("Evernode: Audit request successful."), 0);
                }
                else if (is_audit_success) // Audit success response.
                {
                    // If auditor assigned moment idx is not equal to currect moment start idx.
                    // No host is assigned to audit for this momen.
                    if (lst_moment_start_idx != cur_moment_start_idx)
                        rollback(SBUF("Evernode: No host is assigned to audit for this moment."), 1);

                    // Take the last reward moment of the assigned host.
                    HOST_ADDR_KEY(lst_host_addr_ptr);
                    uint8_t host_addr_buf[HOST_ADDR_VAL_SIZE]; // <host_id(4)><hosting_token(3)><audit_assigned_moment_start_idx(8)><rewarded_moment_start_idx(8)>
                    if (state(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                        rollback(SBUF("Evernode: Host is not registered."), 1);

                    // If host is not assigned for audit in this moment we won't reward.
                    if (UINT64_FROM_BUF(&host_addr_buf[7]) != cur_moment_start_idx)
                        rollback(SBUF("Evernode: Picked host is not assigned for audit in moment."), 1);

                    // If host is already rewarded within this moment we won't reward again.
                    if (UINT64_FROM_BUF(&host_addr_buf[15]) == cur_moment_start_idx)
                        rollback(SBUF("Evernode: The host is already rewarded within this moment."), 1);

                    // Take the host rewards per moment from the config.
                    uint16_t conf_reward;
                    GET_CONF_VALUE(conf_reward, DEF_REWARD, CONF_REWARD, "Evernode: Could not set default state for host reward.");
                    TRACEVAR(conf_reward);

                    // Reward the host.
                    // Reserving one transaction.
                    etxn_reserve(1);

                    // Forward hosting tokens to the host on success.
                    int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                    // Prepare currency.
                    uint8_t amt_out[AMOUNT_BUF_SIZE];
                    // Reward amount would be, total reward amount equally divided by registered host count.
                    int64_t reward_amount = float_divide(float_set(0, conf_reward), float_set(0, host_count));

                    // We need to dump the iou amount into a buffer.
                    // by supplying -1 as the fieldcode we tell float_sto not to prefix an actual STO header on the field.
                    if (float_sto(SBUF(amt_out), SBUF(evr_currency), SBUF(hook_accid), reward_amount, -1) < 0)
                        rollback(SBUF("Evernode: Could not dump reward amount into sto"), 1);

                    // Set the currency code and issuer in the amount field
                    for (int i = 0; GUARD(20), i < 20; ++i)
                    {
                        amt_out[i + 28] = hook_accid[i];
                        amt_out[i + 8] = evr_currency[i];
                    }

                    // Create the outgoing hosting token txn.
                    uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                    PREPARE_PAYMENT_SIMPLE_TRUSTLINE(txn_out, amt_out, fee, lst_host_addr_ptr, 0, 0);

                    uint8_t emithash[HASH_SIZE];
                    if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting txn failed"), 1);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);

                    // Update the auditor state.
                    // Empty the audit details.
                    for (int i = 0; GUARD(28), i < 28; ++i)
                        auditor_addr_buf[i + 4] = 0;
                    if (state_set(SBUF(auditor_addr_buf), SBUF(STP_AUDITOR_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not update state for auditor_addr."), 1);

                    // Update the host's audit state.
                    uint8_t cur_moment_start_idx_buf[8];
                    UINT64_TO_BUF(cur_moment_start_idx_buf, cur_moment_start_idx);
                    for (int i = 0; GUARD(8), i < 8; ++i)
                        host_addr_buf[i + 15] = cur_moment_start_idx_buf[i];
                    if (state_set(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not update audit moment for host_addr."), 1);

                    accept(SBUF("Evernode: Audit success response successful."), 0);
                }
            }

            int is_host_de_reg = 0;
            BUFFER_EQUAL_STR_GUARD(is_host_de_reg, type_ptr, type_len, HOST_DE_REG, 1);
            if (is_host_de_reg)
            {
                // Host de register is only served if at least one host is registered.
                // If host count state does not exist, set host count to 0.
                uint8_t host_count_buf[4] = {0};
                uint32_t host_count = 0;
                if (state(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) != DOESNT_EXIST)
                    host_count = UINT32_FROM_BUF(host_count_buf);
                if (host_count == 0)
                    rollback(SBUF("Evernode: No hosts registered to de register."), 1);

                HOST_ADDR_KEY(account_field);
                // Check whether the host is registered.
                uint8_t host_addr_data[HOST_ADDR_VAL_SIZE]; // <host_id(4)><hosting_token(3)><audit_assigned_moment_start_idx(8)><rewarded_moment_start_idx(8)>
                if (state(SBUF(host_addr_data), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: This host is not registered."), 1);

                // Reserving two transaction.
                etxn_reserve(2);

                uint8_t hosting_token[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, host_addr_data[4], host_addr_data[5], host_addr_data[6], 0, 0, 0, 0, 0};
                uint8_t keylet[34];
                if (util_keylet(SBUF(keylet), KEYLET_LINE, SBUF(hook_accid), SBUF(account_field), SBUF(hosting_token)) != 34)
                    rollback(SBUF("Evernode: Internal error, could not generate keylet for host deregistration"), 1);

                int64_t hook_host_trustline_slot = slot_set(SBUF(keylet), 0);
                if (hook_host_trustline_slot < 0)
                    rollback(SBUF("Evernode: Hook must have a trustline set for HTK to this account."), 1);

                int compare_result = 0;
                ACCOUNT_COMPARE(compare_result, hook_accid, account_field);
                if (compare_result == 0)
                    rollback(SBUF("Evernode: Invalid trustline set hi=lo?"), 1);

                int64_t balance_slot = slot_subfield(hook_host_trustline_slot, sfBalance, 0);
                if (balance_slot < 0)
                    rollback(SBUF("Evernode: Could not find sfBalance on trustline"), 1);

                TRACEVAR(balance_slot);

                int64_t balance_float = slot_float(balance_slot);
                TRACEVAR(balance_float);
                if (balance_float < 0 && balance_float != -29)
                    rollback(SBUF("Evernode: Could not parse user trustline balance."), 1);

                uint8_t amt_out[AMOUNT_BUF_SIZE];
                if (balance_float != -29) // It gives -29 (EXPONENT_UNDERSIZED) when balance is zero. Need to read and experiment more.
                {
                    // Return the available balance to the host before de-registering.

                    // Setup the outgoing txn.
                    int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                    // We need to dump the iou amount into a buffer.
                    // By supplying -1 as the fieldcode we tell float_sto not to prefix an actual STO header on the field.
                    if (float_sto(SBUF(amt_out), SBUF(hosting_token), SBUF(account_field), balance_float, -1) < 0)
                        rollback(SBUF("Evernode: Could not dump hosting token amount into sto for de-registration."), 1);

                    // Set the currency code and issuer in the amount field.
                    for (int i = 0; GUARD(20), i < 20; ++i)
                    {
                        amt_out[i + 28] = account_field[i];
                        amt_out[i + 8] = hosting_token[i];
                    }

                    // Finally create the outgoing txn.
                    uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                    PREPARE_PAYMENT_SIMPLE_TRUSTLINE(txn_out, amt_out, fee, account_field, 0, 0);

                    uint8_t emithash[HASH_SIZE];
                    if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting remaining token transaction failed."), 1);

                    trace(SBUF("returned tx hash: "), SBUF(emithash), 1);
                }

                // Clear amt_out buffer before re-using it.
                CLEARBUF(amt_out);

                FLOAT_ZERO_AMT(amt_out);

                // Set the currency code and issuer in the amount field.
                for (int i = 0; GUARD(20), i < 20; ++i)
                {
                    amt_out[i + 28] = account_field[i];
                    amt_out[i + 8] = hosting_token[i];
                }

                int64_t fee = etxn_fee_base(PREPARE_SIMPLE_TRUSTLINE_SIZE);

                // Preparing trustline transaction.
                uint8_t txn_out[PREPARE_SIMPLE_TRUSTLINE_SIZE];
                PREPARE_SIMPLE_TRUSTLINE(txn_out, amt_out, fee);

                uint8_t emithash[HASH_SIZE];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting txn failed in token 0 trustline."), 1);

                trace(SBUF("emit hash token 0: "), SBUF(emithash), 1);

                // Cleanup the state objects.
                if (host_count > 1)
                {
                    uint8_t last_host_addr[20];
                    uint8_t last_host_id_buf[HOST_ADDR_VAL_SIZE];
                    // Get the address of the host with last host id which is equal to the host count.
                    HOST_ID_KEY(host_count_buf);
                    if (state(SBUF(last_host_addr), SBUF(STP_HOST_ID)) != 20)
                        rollback(SBUF("Evernode: Could not get last host address."), 1);

                    HOST_ADDR_KEY(last_host_addr);
                    if (state(SBUF(last_host_id_buf), SBUF(STP_HOST_ADDR)) != HOST_ADDR_VAL_SIZE)
                        rollback(SBUF("Evernode: Could not get last host id data."), 1);

                    // Update the last host entry with the deleting host id.
                    last_host_id_buf[0] = host_addr_data[0];
                    last_host_id_buf[1] = host_addr_data[1];
                    last_host_id_buf[2] = host_addr_data[2];
                    last_host_id_buf[3] = host_addr_data[3];

                    HOST_ID_KEY(host_addr_data);
                    if (state_set(SBUF(last_host_id_buf), SBUF(STP_HOST_ADDR)) < 0 || state_set(SBUF(last_host_addr), SBUF(STP_HOST_ID)) < 0)
                        rollback(SBUF("Evernode: Could not update state for host_id."), 1);
                }

                // Delete entries.
                HOST_ADDR_KEY(account_field);
                HOST_ID_KEY(host_count_buf);
                if (state_set(0, 0, SBUF(STP_HOST_ADDR)) < 0 || state_set(0, 0, SBUF(STP_HOST_ID)) < 0)
                    rollback(SBUF("Evernode: Could not delete host entries in host de-registration."), 1);

                host_count -= 1;
                UINT32_TO_BUF(host_count_buf, host_count);
                if (state_set(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) < 0)
                    rollback(SBUF("Evernode: Could not reduce host count in host de-registration."), 1);

                accept(SBUF("Evernode: Host de-registration successful."), 0);
            }

            accept(SBUF("Evernode: XRP transaction."), 0);
        }
        else
        {
            // IOU payment.
            int64_t amt = slot_float(amt_slot);
            if (amt < 0)
                rollback(SBUF("Evernode: Could not parse amount."), 1);

            uint8_t amount_buffer[AMOUNT_BUF_SIZE];
            if (slot(SBUF(amount_buffer), amt_slot) != AMOUNT_BUF_SIZE)
                rollback(SBUF("Evernode: Could not dump sfAmount"), 1);

            // Get amount received in drops
            int64_t amount_val_drops = float_int(amt, 6, 0);
            TRACEVAR(amount_val_drops);

            int is_evr;
            IS_EVR(is_evr, amount_buffer, evr_currency, hook_accid);

            // Start filtering from memos type.
            int is_host_reg_req = 0;
            BUFFER_EQUAL_STR_GUARD(is_host_reg_req, type_ptr, type_len, HOST_REG, 1);
            if (is_host_reg_req)
            {
                // Currency should be EVR.
                if (!is_evr)
                    rollback(SBUF("Evernode: Currency should be EVR for host registration."), 1);

                // Take the host reg fee from config.
                uint16_t conf_host_reg_fee;
                GET_CONF_VALUE(conf_host_reg_fee, DEF_HOST_REG_FEE, CONF_HOST_REG_FEE, "Evernode: Could not set default state for host reg fee.");
                TRACEVAR(conf_host_reg_fee);

                if (amount_val_drops < (conf_host_reg_fee * 1000000))
                    rollback(SBUF("Evernode: Amount sent is less than the minimum fee for host registration."), 1);

                // Checking whether this host is already registered.
                HOST_ADDR_KEY(account_field);
                uint8_t host_addr[HOST_ADDR_VAL_SIZE]; // <host_id(4)><hosting_token(3)><audit_assigned_moment_start_idx(8)><rewarded_moment_start_idx(8)>

                if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) != DOESNT_EXIST)
                    rollback(SBUF("Evernode: Host already registered."), 1);

                int is_format_match = 0;
                BUFFER_EQUAL_STR_GUARD(is_format_match, format_ptr, format_len, FORMAT_TEXT, 1);
                if (!is_format_match)
                    rollback(SBUF("Evernode: Memo format should be text."), 50);

                // Generate transaction with following properties.
                /**
                    Transaction type: Trust Set
                    Source: Hook
                    Destination: Host
                    Currency: hosting_token
                    Limit: 999999999 
                    */

                // Reserving one transaction.
                etxn_reserve(1);
                // Calculate fee for trustline transaction.
                int64_t fee = etxn_fee_base(PREPARE_SIMPLE_TRUSTLINE_SIZE);

                uint8_t hosting_token[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, data_ptr[0], data_ptr[1], data_ptr[2], 0, 0, 0, 0, 0};

                uint8_t amt_out[AMOUNT_BUF_SIZE];
                int64_t token_limit = float_sum(float_set(9, 1), float_negate(float_one())); // 999999999
                // we need to dump the iou amount into a buffer
                // by supplying -1 as the fieldcode we tell float_sto not to prefix an actual STO header on the field
                if (float_sto(SBUF(amt_out), SBUF(hosting_token), SBUF(account_field), token_limit, -1) < 0)
                    rollback(SBUF("Evernode: Could not dump hosting token amount into sto"), 1);

                // set the currency code and issuer in the amount field
                for (int i = 0; GUARD(20), i < 20; ++i)
                {
                    amt_out[i + 28] = account_field[i];
                    amt_out[i + 8] = hosting_token[i];
                }

                // Preparing trustline transaction.
                uint8_t txn_out[PREPARE_SIMPLE_TRUSTLINE_SIZE];
                PREPARE_SIMPLE_TRUSTLINE(txn_out, amt_out, fee);

                uint8_t emithash[HASH_SIZE];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting txn failed"), 1);

                trace(SBUF("emit hash: "), SBUF(emithash), 1);

                // If host count state does not exist, set host count to 0.
                uint8_t host_count_buf[4] = {0};
                uint32_t host_count = 0;
                if (state(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) != DOESNT_EXIST)
                    host_count = UINT32_FROM_BUF(host_count_buf);

                uint32_t host_id = host_count + 1;
                uint8_t host_id_arr[4];
                UINT32_TO_BUF(host_id_arr, host_id);
                HOST_ID_KEY(host_id_arr);
                if (state_set(SBUF(account_field), SBUF(STP_HOST_ID)) < 0)
                    rollback(SBUF("Evernode: Could not set state for host_id."), 1);

                host_addr[0] = host_id_arr[0];
                host_addr[1] = host_id_arr[1];
                host_addr[2] = host_id_arr[2];
                host_addr[3] = host_id_arr[3];
                host_addr[4] = data_ptr[0];
                host_addr[5] = data_ptr[1];
                host_addr[6] = data_ptr[2];
                if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                    rollback(SBUF("Evernode: Could not set state for host_addr."), 1);

                host_count += 1;
                UINT32_TO_BUF(host_count_buf, host_count);
                if (state_set(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) < 0)
                    rollback(SBUF("Evernode: Could not set default state for host count."), 1);

                accept(SBUF("Host registration successful."), 0);
            }

            int is_redeem_req = 0;
            BUFFER_EQUAL_STR_GUARD(is_redeem_req, type_ptr, type_len, REDEEM, 1);
            if (is_redeem_req)
            {
                if (is_evr)
                    rollback(SBUF("Evernode: Currency cannot be EVR for redeem request."), 1);

                int is_format_match = 0;
                BUFFER_EQUAL_STR_GUARD(is_format_match, format_ptr, format_len, FORMAT_BINARY, 1);
                if (!is_format_match)
                    rollback(SBUF("Evernode: Memo format should be binary."), 50);

                uint8_t *issuer_ptr = &amount_buffer[28];

                // Checking whether this host is registered.
                HOST_ADDR_KEY(issuer_ptr);
                uint8_t host_addr[HOST_ADDR_VAL_SIZE]; // <host_id(4)><hosting_token(3)><audit_assigned_moment_start_idx(8)><rewarded_moment_start_idx(8)>

                if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: Host is not registered."), 1);

                // Checking whether transaction is with host tokens
                uint8_t hosting_token[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, host_addr[4], host_addr[5], host_addr[6], 0, 0, 0, 0, 0};
                uint8_t is_hosting_token = 0;
                BUFFER_EQUAL_GUARD(is_hosting_token, hosting_token, 20, &amount_buffer[8], 20, 20);
                if (!is_hosting_token)
                    rollback(SBUF("Evernode: Currency should be in hosting tokens to redeem."), 1);

                // Take the minimum redeem amount from the config.
                uint16_t conf_min_redeem;
                GET_CONF_VALUE(conf_min_redeem, DEF_MIN_REDEEM, CONF_MIN_REDEEM, "Evernode: Could not set default state for min redeem.");
                TRACEVAR(conf_min_redeem);

                if (amount_val_drops < (conf_min_redeem * 1000000))
                    rollback(SBUF("Evernode: Amount sent is less than the minimum fee."), 1);

                // Get transaction hash(id).
                uint8_t txid[HASH_SIZE];
                int32_t txid_len = otxn_id(SBUF(txid), 0);
                if (txid_len < HASH_SIZE)
                    rollback(SBUF("Evernode: transaction id missing!!!"), 10);

                // Prepare state value.
                uint8_t redeem_op[REDEEM_STATE_VAL_SIZE];

                // Set the host token.
                redeem_op[0] = amount_buffer[20];
                redeem_op[1] = amount_buffer[21];
                redeem_op[2] = amount_buffer[22];

                // Set the amount.
                uint8_t amount_buf[8];
                INT64_TO_BUF(amount_buf, amt);
                for (int i = 0; GUARD(8), i < 8; ++i)
                    redeem_op[i + 3] = amount_buf[i];

                // Set the issuer.
                for (int i = 0; GUARD(20), i < 20; ++i)
                    redeem_op[i + 11] = issuer_ptr[i];

                // Set the ledger.
                int64_t ledger = ledger_seq();
                uint8_t ledger_buf[8];
                INT64_TO_BUF(ledger_buf, ledger);
                for (int i = 0; GUARD(8), i < 8; ++i)
                    redeem_op[i + 31] = ledger_buf[i];

                // Set state key with transaction hash(id).
                REDEEM_OP_KEY(txid);
                if (state_set(SBUF(redeem_op), SBUF(STP_REDEEM_OP)) < 0)
                    rollback(SBUF("Evernode: Could not set state for redeem_op."), 1);

                accept(SBUF("Redeem request successful."), 0);
            }
        }
    }

    accept(SBUF("Evernode: Transaction type not supported."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
