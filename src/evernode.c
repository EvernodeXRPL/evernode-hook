#include "../lib/hookapi.h"
#include "constants.h"
#include "evernode.h"
#include "statekeys.h"

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

    int64_t txn_type = otxn_type();
    if (txn_type == ttPAYMENT)
    {
        int64_t cur_ledger_seq = ledger_seq();

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

        uint8_t *memo_ptr, *type_ptr, *format_ptr, *data_ptr;
        uint32_t memo_len, type_len, format_len, data_len;
        GET_MEMO(0, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len);

        if (is_xrp)
        {
            // Redeem responses.
            int is_redeem_suc = 0, is_redeem_err = 0;
            BUFFER_EQUAL_STR_GUARD(is_redeem_suc, type_ptr, type_len, REDEEM_SUCCESS, 1);
            if (!is_redeem_suc)
                BUFFER_EQUAL_STR_GUARD(is_redeem_err, type_ptr, type_len, REDEEM_ERROR, 1);

            if (is_redeem_suc || is_redeem_err)
            {
                if (is_redeem_suc)
                {
                    BUFFER_EQUAL_STR_GUARD(is_redeem_suc, format_ptr, format_len, FORMAT_BASE64, 1);
                    if (!is_redeem_suc)
                        rollback(SBUF("Evernode: Redeem success memo format should be base64."), 50);
                }
                else
                {
                    BUFFER_EQUAL_STR_GUARD(is_redeem_err, format_ptr, format_len, FORMAT_JSON, 1);
                    if (!is_redeem_err)
                        rollback(SBUF("Evernode: Redeem error memo format should be text/json."), 50);
                }

                uint8_t redeem_res[data_len];
                COPY_BUF(redeem_res, 0, data_ptr, 0, data_len);

                // Redeem response has 2 memos, so check for the type in second memo.
                GET_MEMO(1, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len);

                int is_redeem_ref = 0;
                BUFFER_EQUAL_STR_GUARD(is_redeem_ref, type_ptr, type_len, REDEEM_REF, 1);
                if (!is_redeem_ref)
                    rollback(SBUF("Evernode: Redeem response does not have a reference."), 1);

                BUFFER_EQUAL_STR_GUARD(is_redeem_ref, format_ptr, format_len, FORMAT_HEX, 1);
                if (!is_redeem_ref)
                    rollback(SBUF("Evernode: Redeem reference memo format should be hex."), 50);

                uint8_t redeem_ref[data_len];
                COPY_BUF(redeem_ref, 0, data_ptr, 0, data_len);

                REDEEM_OP_KEY(redeem_ref);

                uint8_t redeem_op[REDEEM_STATE_VAL_SIZE]; // <hosting_token(3)><amount(8)><host_addr(20)><lcl_index(8)><user_addr(20)>
                if (state(SBUF(redeem_op), SBUF(STP_REDEEM_OP)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: No redeem state for the redeem response."), 1);

                uint8_t *useraddr_ptr = &redeem_op[39];

                // If redeem succeed we have to prepare two transactions.
                if (is_redeem_suc)
                    etxn_reserve(2);
                else
                    etxn_reserve(1);

                // Forward redeem res to the user
                int64_t fee = etxn_fee_base(PREPARE_PAYMENT_REDEEM_RESP_SIZE(sizeof(redeem_res), sizeof(redeem_ref), is_redeem_suc));

                uint8_t txn_out[PREPARE_PAYMENT_REDEEM_RESP_SIZE(sizeof(redeem_res), sizeof(redeem_ref), is_redeem_suc)];
                PREPARE_PAYMENT_REDEEM_RESP_M(txn_out, MIN_DROPS, fee, useraddr_ptr, redeem_res, sizeof(redeem_res), redeem_ref, sizeof(redeem_ref), is_redeem_suc, 10);

                uint8_t emithash[HASH_SIZE];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting user txn failed"), 1);
                trace(SBUF("emit user hash: "), SBUF(emithash), 1);

                // Send hosting tokens to the host and clear the state only if there's no error.
                if (is_redeem_suc)
                {
                    int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                    // Prepare currency.
                    uint8_t *host_amount_ptr = &redeem_op[3];
                    int64_t host_amount = INT64_FROM_BUF(host_amount_ptr);
                    uint8_t *issuer_ptr = &redeem_op[11];

                    uint8_t amt_out[AMOUNT_BUF_SIZE];
                    SET_AMOUNT_OUT(amt_out, redeem_op, issuer_ptr, host_amount);

                    // Create the outgoing hosting token txn.
                    uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                    PREPARE_PAYMENT_SIMPLE_TRUSTLINE(txn_out, amt_out, fee, account_field, 0, 0);

                    uint8_t emithash[HASH_SIZE];
                    if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting txn failed"), 1);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);

                    if (state_set(0, 0, SBUF(STP_REDEEM_OP)) < 0)
                        rollback(SBUF("Evernode: Could not delete state for redeem_op."), 1);

                    // Decreasing the locked amount. since host token are forwarded.
                    HOST_ADDR_KEY(issuer_ptr);
                    // <host_id(4)><hosting_token(3)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><audit_assigned_moment_start_idx(8)>
                    // <auditor_addr(20)><rewarded_moment_start_idx(8)><accumulated_rewards(8)><locked_token_amont(8)><last_hearbeat_ledger_idx(8)>
                    uint8_t host_addr[HOST_ADDR_VAL_SIZE];
                    if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                        rollback(SBUF("Evernode: This host is not registered."), 1);

                    uint8_t *locked_amount_ptr = &host_addr[HOST_LOCKED_TOKEN_AMT_OFFSET];
                    int64_t cur_locked_amount = INT64_FROM_BUF(locked_amount_ptr);
                    cur_locked_amount = float_sum(cur_locked_amount, float_negate(host_amount));
                    // Float arithmatic sometimes gives -29 for 0, So we manually override float to 0.
                    if (IS_FLOAT_ZERO(cur_locked_amount))
                        cur_locked_amount = 0;

                    INT64_TO_BUF(locked_amount_ptr, cur_locked_amount);
                    if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not set state for the locked amount."), 1);

                    accept(SBUF("Redeem response successful."), 0);
                }
                else
                    // If error refund the host tokens back to user.
                    accept(SBUF("Redeem response failed."), 1);
            }

            // Refund.
            int is_refund_req = 0;
            BUFFER_EQUAL_STR_GUARD(is_refund_req, type_ptr, type_len, REFUND, 1);
            if (is_refund_req)
            {
                BUFFER_EQUAL_STR_GUARD(is_refund_req, format_ptr, format_len, FORMAT_HEX, 1);
                if (!is_refund_req)
                    rollback(SBUF("Evernode: Memo format should be hex in refund request."), 1);

                if (data_len != 32) // 32 bytes is the size of the hash in hex
                    rollback(SBUF("Evernode: Memo data should be 64 bytes in hex in refund request."), 1);

                uint8_t refund_ref[data_len];
                COPY_BUF(refund_ref, 0, data_ptr, 0, data_len);
                REDEEM_OP_KEY(refund_ref);

                // Get transaction hash(id).
                uint8_t txid[HASH_SIZE];
                int32_t txid_len = otxn_id(SBUF(txid), 0);
                if (txid_len < HASH_SIZE)
                    rollback(SBUF("Evernode: transaction id missing!!!"), 1);

                uint8_t data_arr[REDEEM_STATE_VAL_SIZE]; // <hosting_token(3)><amount(8)><host_addr(20)><lcl_index(8)><user_addr(20)>
                if (state(SBUF(data_arr), SBUF(STP_REDEEM_OP)) < 0)
                {
                    // Setup the outgoing txn.
                    // Reserving one transaction.
                    etxn_reserve(1);
                    int64_t fee = etxn_fee_base(PREPARE_PAYMENT_REFUND_ERROR_SIZE);

                    // Finally create the outgoing txn.
                    uint8_t txn_out[PREPARE_PAYMENT_REFUND_ERROR_SIZE];
                    PREPARE_PAYMENT_REFUND_ERROR_M(txn_out, MIN_DROPS, fee, account_field, txid, 10);

                    uint8_t emithash[HASH_SIZE];
                    if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting refund transaction failed."), 1);

                    trace(SBUF("refund error tx hash: "), SBUF(emithash), 1);

                    accept(SBUF("Evernode: No redeem for this tx hash."), 1);
                }
                else
                {
                    // Take the redeem window from the config.
                    uint16_t conf_redeem_window;
                    GET_CONF_VALUE(conf_redeem_window, DEF_REDEEM_WINDOW, CONF_REDEEM_WINDOW, "Evernode: Could not set default state for redeem window.");
                    TRACEVAR(conf_redeem_window);

                    uint8_t *ptr = &data_arr[31];
                    int64_t ledger_seq_def = cur_ledger_seq - INT64_FROM_BUF(ptr);
                    if (ledger_seq_def < conf_redeem_window)
                        rollback(SBUF("Evernode: Redeeming window is not yet passed. Rejected."), 1);

                    uint8_t *issuer_ptr = &data_arr[11];
                    uint8_t *amount_ptr = &data_arr[3];
                    int64_t token_amount = INT64_FROM_BUF(amount_ptr);

                    uint8_t amt_out[AMOUNT_BUF_SIZE];
                    SET_AMOUNT_OUT(amt_out, data_arr, issuer_ptr, token_amount);

                    // Setup the outgoing txn.
                    // Reserving one transaction.
                    etxn_reserve(1);
                    int64_t fee = etxn_fee_base(PREPARE_PAYMENT_REFUND_SUCCESS_SIZE);

                    // Finally create the outgoing txn.
                    uint8_t txn_out[PREPARE_PAYMENT_REFUND_SUCCESS_SIZE];
                    PREPARE_PAYMENT_REFUND_SUCCESS_M(txn_out, amt_out, fee, account_field, txid, refund_ref, 10);

                    uint8_t emithash[HASH_SIZE];
                    if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting refund transaction failed."), 1);

                    trace(SBUF("refund tx hash: "), SBUF(emithash), 1);

                    // Delete state entry after refund.
                    if (state_set(0, 0, SBUF(STP_REDEEM_OP)) < 0)
                        rollback(SBUF("Evernode: Could not delete state entry after refund."), 1);

                    // Decreasing the locked amount. since host token are refunded.
                    HOST_ADDR_KEY(issuer_ptr);
                    // <host_id(4)><hosting_token(3)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><audit_assigned_moment_start_idx(8)>
                    // <auditor_addr(20)><rewarded_moment_start_idx(8)><accumulated_rewards(8)><locked_token_amont(8)><last_hearbeat_ledger_idx(8)>
                    uint8_t host_addr[HOST_ADDR_VAL_SIZE];
                    if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                        rollback(SBUF("Evernode: This host is not registered."), 1);

                    uint8_t *locked_amount_ptr = &host_addr[HOST_LOCKED_TOKEN_AMT_OFFSET];
                    int64_t cur_locked_amount = INT64_FROM_BUF(locked_amount_ptr);
                    cur_locked_amount = float_sum(cur_locked_amount, float_negate(token_amount));
                    // Float arithmatic sometimes gives -29 for 0, So we manually override float to 0.
                    if (IS_FLOAT_ZERO(cur_locked_amount))
                        cur_locked_amount = 0;

                    INT64_TO_BUF(locked_amount_ptr, cur_locked_amount);
                    if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not set state for the locked amount."), 1);

                    accept(SBUF("Evernode: Refund operation successful."), 0);
                }
            }

            // Audit request or Audit success response.
            int is_audit_req = 0, is_audit_suc = 0, is_audit_fail = 0;
            BUFFER_EQUAL_STR_GUARD(is_audit_req, type_ptr, type_len, AUDIT, 1);
            if (!is_audit_req)
                BUFFER_EQUAL_STR_GUARD(is_audit_suc, type_ptr, type_len, AUDIT_SUCCESS, 1);
            if (!(is_audit_req || is_audit_suc))
                BUFFER_EQUAL_STR_GUARD(is_audit_fail, type_ptr, type_len, AUDIT_FAILED, 1);

            if (is_audit_req || is_audit_suc || is_audit_fail)
            {
                // Common checks for both audit request and audit suceess response.
                if (is_audit_suc)
                {
                    BUFFER_EQUAL_STR_GUARD(is_audit_suc, format_ptr, format_len, FORMAT_HEX, 1);
                    if (!is_audit_suc)
                        rollback(SBUF("Evernode: Audit success memo format should be hex."), 50);
                }
                else if (is_audit_fail)
                {
                    BUFFER_EQUAL_STR_GUARD(is_audit_fail, format_ptr, format_len, FORMAT_HEX, 1);
                    if (!is_audit_fail)
                        rollback(SBUF("Evernode: Audit failure memo format should be hex."), 50);
                }

                // Audit request is only served if at least one host is registered.
                uint8_t host_count_buf[4];
                uint32_t host_count;
                GET_HOST_COUNT(host_count_buf, host_count);
                if (host_count == 0)
                    rollback(SBUF("Evernode: No hosts registered to audit."), 1);

                // If default auditor is not set, first set the default auditor.
                uint8_t auditor_count_buf[4] = {0};
                if (state(SBUF(auditor_count_buf), SBUF(STK_AUDITOR_COUNT)) == DOESNT_EXIST)
                {
                    // Setting up default auditor if no auditors registered.
                    uint8_t auditor_accid[20];
                    util_accid(SBUF(auditor_accid), SBUF(DEF_AUDITOR_ADDR));
                    uint8_t auditor_id_buf[4];
                    // Id of the default auditor is 1.
                    UINT32_TO_BUF(auditor_id_buf, 1);
                    AUDITOR_ID_KEY(auditor_id_buf);
                    if (state_set(SBUF(auditor_accid), SBUF(STP_AUDITOR_ID)) < 0)
                        rollback(SBUF("Evernode: Could not set state for default auditor_id."), 1);

                    uint8_t auditor_addr_buf[AUDITOR_ADDR_VAL_SIZE] = {0};
                    COPY_BUF(auditor_addr_buf, 0, auditor_id_buf, 0, 4);
                    AUDITOR_ADDR_KEY(auditor_accid);
                    if (state_set(SBUF(auditor_addr_buf), SBUF(STP_AUDITOR_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not set state for default auditor_addr."), 1);

                    // Set auditor count to 1;
                    UINT32_TO_BUF(auditor_count_buf, 1);
                    if (state_set(SBUF(auditor_count_buf), SBUF(STK_AUDITOR_COUNT)) < 0)
                        rollback(SBUF("Evernode: Could not set default state for auditor count."), 1);
                }
                uint32_t auditor_count = UINT32_FROM_BUF(auditor_count_buf);

                // Checking whether this auditor exists.
                AUDITOR_ADDR_KEY(account_field);
                uint8_t auditor_addr_buf[AUDITOR_ADDR_VAL_SIZE]; // <auditor_id(4)><moment_start_idx(8)>
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
                uint64_t cur_moment_start_idx;
                GET_MOMENT_START_INDEX_MOMENT_BASE_SIZE_GIVEN(cur_moment_start_idx, cur_ledger_seq, moment_base_idx, conf_moment_size);

                // We do not serve audit requests if moment start index is 0.
                if (cur_moment_start_idx == 0)
                    rollback(SBUF("Evernode: Rewards aren't allowed in the first moment."), 1);

                uint32_t auditor_id = UINT32_FROM_BUF(auditor_addr_buf);
                uint64_t lst_moment_start_idx = UINT64_FROM_BUF(&auditor_addr_buf[4]);

                // Seperate logic for audit request and audit suceess response.
                if (is_audit_req) // Audit request
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

                    // Take the max reward from the config.
                    uint16_t conf_max_reward;
                    GET_CONF_VALUE(conf_max_reward, DEF_MAX_REWARD, CONF_MAX_REWARD, "Evernode: Could not set default state for max reward.");
                    TRACEVAR(conf_max_reward);

                    // Take the max audit from the config.
                    uint16_t conf_max_audit;
                    GET_CONF_VALUE(conf_max_audit, DEF_MAX_AUDIT, CONF_MAX_AUDIT, "Evernode: Could not set default state for max audit.");
                    TRACEVAR(conf_max_audit);

                    // Take the minimum redeem amount from the config.
                    uint16_t conf_min_redeem;
                    GET_CONF_VALUE(conf_min_redeem, DEF_MIN_REDEEM, CONF_MIN_REDEEM, "Evernode: Could not set default state for min redeem.");
                    TRACEVAR(conf_min_redeem);

                    // Take the host rewards per moment from the config.
                    int64_t conf_reward;
                    GET_FLOAT_CONF_VALUE(conf_reward, DEF_REWARD_M, DEF_REWARD_E, CONF_REWARD, "Evernode: Could not set default state for host reward.");
                    TRACEVAR(conf_reward);

                    // Setting the accumulated rewards.

                    // Take the last accumulated moment from the config.
                    uint8_t last_updated_moment_start_idx_buf[8] = {0};
                    uint64_t last_updated_moment_start_idx = 0;
                    if (state(SBUF(last_updated_moment_start_idx_buf), SBUF(STK_ACCUMULATED_MOMENT_IDX)) != DOESNT_EXIST)
                        last_updated_moment_start_idx = UINT64_FROM_BUF(last_updated_moment_start_idx_buf);

                    // Consider only active hosts. Loop through all the host and check for activeness.
                    // Prepare an array to keep active hosts.
                    // <host1_addr(20)><host2_addr(20)><host3_addr(20)>....
                    // This buf can populate upto maximum host_count, So we pre allocate array for that limit.
                    uint32_t host_info_len = 20;
                    uint32_t active_host_addr_buf[host_count * host_info_len];
                    uint32_t active_host_count = 0;
                    for (int i = 0; GUARD(host_count), i < host_count; ++i)
                    {
                        uint8_t host_addr[20];
                        uint8_t host_id_arr[4];
                        UINT32_TO_BUF(host_id_arr, i + 1);
                        HOST_ID_KEY_GUARD(host_id_arr, host_count);
                        if (state(SBUF(host_addr), SBUF(STP_HOST_ID)) == DOESNT_EXIST)
                            rollback(SBUF("Evernode: Could not find a matching host for the id."), 1);

                        // Take the last audit assigned moment.
                        HOST_ADDR_KEY_GUARD(host_addr, host_count);
                        // <host_id(4)><hosting_token(3)><instance_size(60)><location(10)><audit_assigned_moment_start_idx(8)><auditor_addr(20)><rewarded_moment_start_idx(8)>
                        uint8_t host_addr_buf[HOST_ADDR_VAL_SIZE];
                        if (state(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                            rollback(SBUF("Evernode: Host is not registered."), 1);

                        int is_active = 0;
                        IS_HOST_ACTIVE_MOMENT_IDX_SIZE_GIVEN(is_active, host_addr_buf, cur_moment_start_idx, conf_moment_size);
                        if (is_active)
                        {
                            uint8_t *buf_ptr = &active_host_addr_buf[host_info_len * active_host_count];
                            // Copy address to the buffer.
                            COPY_BUF_GUARD(buf_ptr, 0, host_addr, 0, 20, host_count);
                            active_host_count++;
                        }

                        // If the host is currently inactive, But it has rewards to accumulate for last active moments,
                        // We can consider this when last heartbeat moment start idx > last updated moment start idx,
                        // Then this host has missing moments of (moment start idx - last updated moment start idx).
                        // But we don't know active hosts at that point, so we can't calculate amount to accumulate.
                    }

                    if (active_host_count == 0)
                        rollback(SBUF("Evernode: No active hosts to audit."), 1);

                    uint8_t *moment_seed_ptr = &moment_seed_buf[8];
                    trace(SBUF("Moment seed: "), moment_seed_ptr, HASH_SIZE, 1);

                    // Host index where hosts picking is started.
                    uint32_t pick_start_host_idx = (UINT32_FROM_BUF(moment_seed_ptr) % active_host_count);
                    // Auditor id which starts picking at "pick_start_host_id"
                    uint32_t pick_start_auditor_id = (UINT32_FROM_BUF(moment_seed_ptr + 1) % auditor_count) + 1;
                    // Auditor "pick_start_auditor_id" picks host from "pick_start_host_idx" and upto "pick_start_host_idx + pick_count"
                    // Auditor "pick_start_auditor_id + 1" picks host from "pick_start_host_idx + pick_count" and upto "pick_start_host_idx + (2 * pick_count)"
                    // ...

                    // Relative index of the requested auditor id, Ex: {If "auditor_id = 4", "pick_start_auditor_id = 2" and "auditor_count = 5" then,
                    // "pick_idx = [((5 + 4) - 2) % 5] = 2"}
                    uint32_t pick_idx = ((auditor_count + auditor_id) - pick_start_auditor_id) % auditor_count;
                    // How many hosts will be assigned for the auditor min(ceil(active_host_count / auditor_count), conf_max_audit).
                    // Note: maximum pick count would be conf_max_audit.
                    uint32_t pick_count = MIN(CEIL(active_host_count, auditor_count), conf_max_audit);
                    // Which is the picking starting host index for this auditor.
                    uint32_t pick_host_from_idx = (pick_start_host_idx + (pick_idx * pick_count)) % active_host_count;
                    // If "active_host_count < auditor_count" then "pick_count" would be 1, and extra auditors ("pick_idx >= active_host_count") won't get assigned audits.
                    // If this is the last index, it might not get assigned all the "pick_count" hosts from "pick_host_from".
                    // Beacause when "active_host_count % auditor_count > 0", last auditor gets the remainder.
                    if ((auditor_count > active_host_count) && (pick_idx >= active_host_count))
                        rollback(SBUF("Evernode: No enough hosts to audit for this auditor."), 1);
                    else if ((pick_idx == auditor_count - 1) && (active_host_count % auditor_count > 0))
                        pick_count = active_host_count % auditor_count;
                    // Which is the picking endig host index for this auditor.
                    uint32_t pick_host_to_idx = (pick_host_from_idx + pick_count) % active_host_count;

                    // Setup the outgoing txns for all checkCreates representing hosts assigned for this auditor.
                    etxn_reserve(pick_count);

                    // If there're missed moments, we loop for all the active hosts and distribute accumulated rewards.
                    // And withing same loop, if the active host index is (> pick_host_from_idx) and (< pick_host_from_idx + pick_count) emit the check to the auditor.
                    // If there're no missed moments, we loop for pick_count and emit checks to the auditor from the pick_host_from_idx
                    uint64_t missed_moments = (cur_moment_start_idx - MAX(last_updated_moment_start_idx, moment_base_idx)) / conf_moment_size;
                    if (missed_moments > 0)
                    {
                        // Accumulated amount would be, total reward amount equally divided by active host count multiplied by missed moments.
                        int64_t acc_amount = float_divide(conf_reward, float_set(0, active_host_count));
                        acc_amount = float_multiply(acc_amount, float_set(0, missed_moments));

                        // There can be a possibility of some hosts has gone active or inactive within these missed moments.
                        // In that case amount to accumulate won't be same for all the moment, we need to consider that scenario later.

                        for (int i = 0; GUARD(active_host_count), i < active_host_count; ++i)
                        {
                            uint8_t *host_addr_ptr = &active_host_addr_buf[host_info_len * i];
                            // Take the last audit assigned moment.
                            HOST_ADDR_KEY_GUARD(host_addr_ptr, host_count);
                            // <host_id(4)><hosting_token(3)><instance_size(60)><location(10)><audit_assigned_moment_start_idx(8)><auditor_addr(20)><rewarded_moment_start_idx(8)>
                            uint8_t host_addr_buf[HOST_ADDR_VAL_SIZE];
                            if (state(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                                rollback(SBUF("Evernode: Host is not registered."), 1);

                            uint8_t *cur_acc_amount_ptr = &host_addr_buf[HOST_ACCUMULATED_AMT_OFFSET];
                            int64_t cur_acc_amount = INT64_FROM_BUF(cur_acc_amount_ptr);

                            // If last audit hasn't been rewarded it should be an audit failure.
                            // Then if accumulated amount is > 0 add accumulated amount to the pool.
                            uint64_t audit_assigned_moment_start_idx = UINT64_FROM_BUF(&host_addr_buf[HOST_AUDIT_IDX_OFFSET]);
                            if ((cur_moment_start_idx > audit_assigned_moment_start_idx) &&
                                (cur_acc_amount > 0) &&
                                (audit_assigned_moment_start_idx > UINT64_FROM_BUF(&host_addr_buf[HOST_REWARD_IDX_OFFSET])))
                            {
                                ADD_TO_REWARD_POOL(cur_acc_amount);
                                cur_acc_amount = 0;
                            }

                            // Update the host's accumulated amount state.
                            cur_acc_amount = float_sum(cur_acc_amount, acc_amount);
                            INT64_TO_BUF(cur_acc_amount_ptr, cur_acc_amount);

                            // Check whether this host is to picked by this auditor. If so emit the check.
                            int is_picked = 0;
                            if (pick_host_from_idx == pick_host_to_idx) // If range is like (3 to 3) {0, 1, 2], [3, 4, 5} -> |3, 4, 5, 0, 1, 2|.
                                is_picked = 1;
                            else if (pick_host_from_idx < pick_host_to_idx) // If range is like (1 to 4) {0, [1, 2, 3, 4], 5} -> |1, 2, 3, 4|.
                                is_picked = (i >= pick_host_from_idx && i < pick_host_to_idx);
                            else if (pick_host_from_idx > pick_host_to_idx) // If range is like (4 to 1) {0, 1], 2, 3, [4, 5} -> |4, 5, 0, 1|.
                                is_picked = i >= pick_host_from_idx || i < pick_host_to_idx;

                            if (is_picked)
                                EMIT_AUDIT_CHECK_GUARDM(cur_moment_start_idx, moment_seed_buf, conf_min_redeem, host_addr_ptr, host_addr_buf, account_field, pick_count, 10);

                            if (state_set(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) < 0)
                                rollback(SBUF("Evernode: Could not update audit moment for host_addr."), 1);
                        }

                        // Update the last accumulated moment state.
                        UINT64_TO_BUF(last_updated_moment_start_idx_buf, cur_moment_start_idx);
                        if (state_set(SBUF(last_updated_moment_start_idx_buf), SBUF(STK_ACCUMULATED_MOMENT_IDX)) < 0)
                            rollback(SBUF("Evernode: Could not set default state for last accumulated moment."), 1);
                    }
                    else
                    {
                        // Emit the checks of picked hosts.
                        for (int i = 0; GUARD(pick_count), i < pick_count; ++i)
                        {
                            // Take the host address.
                            uint32_t pick_idx = (pick_host_from_idx + i) % active_host_count;
                            uint8_t *host_addr_ptr = &active_host_addr_buf[host_info_len * pick_idx];

                            // Take the last audit assigned moment.
                            HOST_ADDR_KEY_GUARD(host_addr_ptr, pick_count);
                            // <host_id(4)><hosting_token(3)><instance_size(60)><location(10)><audit_assigned_moment_start_idx(8)><auditor_addr(20)><rewarded_moment_start_idx(8)>
                            uint8_t host_addr_buf[HOST_ADDR_VAL_SIZE];
                            if (state(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                                rollback(SBUF("Evernode: Host is not registered."), 1);

                            EMIT_AUDIT_CHECK_GUARDM(cur_moment_start_idx, moment_seed_buf, conf_min_redeem, host_addr_ptr, host_addr_buf, account_field, pick_count, 10);

                            if (state_set(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) < 0)
                                rollback(SBUF("Evernode: Could not update audit moment for host_addr."), 1);
                        }
                    }

                    // Update the auditor state if at least one host is assigned.
                    if (pick_count > 0)
                    {
                        COPY_BUF(auditor_addr_buf, 4, moment_seed_buf, 0, 8);
                        if (state_set(SBUF(auditor_addr_buf), SBUF(STP_AUDITOR_ADDR)) < 0)
                            rollback(SBUF("Evernode: Could not update state for auditor_addr."), 1);
                    }

                    accept(SBUF("Evernode: Audit request successful."), 0);
                }
                else if (is_audit_suc || is_audit_fail) // Audit success or error response.
                {
                    // If auditor assigned moment idx is not equal to currect moment start idx.
                    // No host is assigned to audit for this moment.
                    if (lst_moment_start_idx != cur_moment_start_idx)
                        rollback(SBUF("Evernode: No host is assigned to audit for this moment."), 1);

                    uint8_t *host_addr_ptr = data_ptr;

                    // Take the last reward moment of the assigned host.
                    HOST_ADDR_KEY(host_addr_ptr);
                    // <host_id(4)><hosting_token(3)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><audit_assigned_moment_start_idx(8)>
                    // <auditor_addr(20)><rewarded_moment_start_idx(8)><accumulated_rewards(8)><locked_token_amont(8)><last_hearbeat_ledger_idx(8)>
                    uint8_t host_addr_buf[HOST_ADDR_VAL_SIZE];
                    if (state(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                        rollback(SBUF("Evernode: Host is not registered."), 1);

                    // If host is not assigned for audit in this moment we won't reward.
                    if (UINT64_FROM_BUF(&host_addr_buf[HOST_AUDIT_IDX_OFFSET]) != cur_moment_start_idx)
                        rollback(SBUF("Evernode: The host isn't assigned for audit in moment."), 1);

                    // If the host is not assigned for audit from this auditor.
                    int is_valid_res = 0;
                    BUFFER_EQUAL_GUARD(is_valid_res, &host_addr_buf[HOST_AUDITOR_OFFSET], 20, account_field, 20, 1);
                    if (!is_valid_res)
                        rollback(SBUF("Evernode: The host isn't assigned for audit from this auditor."), 1);

                    // If host is already rewarded within this moment we won't reward again or fail the audit.
                    if (UINT64_FROM_BUF(&host_addr_buf[HOST_REWARD_IDX_OFFSET]) == cur_moment_start_idx)
                        rollback(SBUF("Evernode: The host is already rewarded within this moment."), 1);

                    // Reward amount would be, total accumulated amount upto now.
                    uint8_t *acc_amount_ptr = &host_addr_buf[HOST_ACCUMULATED_AMT_OFFSET];
                    int64_t reward_amount = INT64_FROM_BUF(acc_amount_ptr);

                    // If reward amount is zero, means accumulated amount is already made zero.
                    // Since host already rewarded check failed in line 544 it means accumulated amout is empied by an audit failure.
                    if (reward_amount == 0)
                        rollback(SBUF("Evernode: An audit for this host has been already failed within this moment."), 1);

                    // Reward the host only if audit success.
                    // Otherwise put reward amount into the reward pool.
                    if (is_audit_suc)
                    {
                        // Reserving one transaction.
                        etxn_reserve(1);

                        // Forward hosting tokens to the host on success.
                        int64_t fee = etxn_fee_base(PREPARE_PAYMENT_REWARD_SIZE);

                        uint8_t amt_out[AMOUNT_BUF_SIZE];
                        SET_AMOUNT_OUT(amt_out, EVR_TOKEN, hook_accid, reward_amount);

                        // Create the outgoing hosting token txn.
                        uint8_t txn_out[PREPARE_PAYMENT_REWARD_SIZE];
                        PREPARE_PAYMENT_REWARD_M(txn_out, amt_out, fee, host_addr_ptr, 10);

                        uint8_t emithash[HASH_SIZE];
                        if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                            rollback(SBUF("Evernode: Emitting txn failed"), 1);
                        trace(SBUF("emit hash: "), SBUF(emithash), 1);

                        // Update the host's audit rewarded state.
                        UINT64_TO_BUF(&host_addr_buf[HOST_REWARD_IDX_OFFSET], cur_moment_start_idx);
                    }
                    else
                        ADD_TO_REWARD_POOL(reward_amount);

                    // Zero the accumulated amount.
                    INT64_TO_BUF(&host_addr_buf[HOST_ACCUMULATED_AMT_OFFSET], 0);
                    if (state_set(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not update audit moment for host_addr."), 1);

                    if (is_audit_suc)
                        accept(SBUF("Evernode: Audit success response successful."), 0);
                    else
                        accept(SBUF("Evernode: Audit failure response successful."), 0);
                }
            }

            // Host deregistration.
            int is_host_de_reg = 0;
            BUFFER_EQUAL_STR_GUARD(is_host_de_reg, type_ptr, type_len, HOST_DE_REG, 1);
            if (is_host_de_reg)
            {
                // Host de register is only served if at least one host is registered.
                uint8_t host_count_buf[4];
                uint32_t host_count;
                GET_HOST_COUNT(host_count_buf, host_count);
                if (host_count == 0)
                    rollback(SBUF("Evernode: No hosts registered to de register."), 1);

                HOST_ADDR_KEY(account_field);
                // Check whether the host is registered.
                // <host_id(4)><hosting_token(3)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><audit_assigned_moment_start_idx(8)>
                // <auditor_addr(20)><rewarded_moment_start_idx(8)><accumulated_rewards(8)><locked_token_amont(8)><last_hearbeat_ledger_idx(8)>
                uint8_t host_addr_data[HOST_ADDR_VAL_SIZE];
                if (state(SBUF(host_addr_data), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: This host is not registered."), 1);

                uint8_t *host_token_ptr = &host_addr_data[HOST_TOKEN_OFFSET];

                // Reserving two transaction.
                etxn_reserve(2);

                uint8_t hosting_token[20] = GET_TOKEN_CURRENCY(host_token_ptr);
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

                uint8_t amt_out[AMOUNT_BUF_SIZE];
                // If value is < 0 or not 0 value is invalid.
                if (balance_float < 0 && !IS_FLOAT_ZERO(balance_float))
                    rollback(SBUF("Evernode: Could not parse user trustline balance."), 1);
                else if (!IS_FLOAT_ZERO(balance_float)) // If we have positive balance.
                {
                    // Return the available balance to the host before de-registering.

                    // Setup the outgoing txn.
                    int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                    uint8_t amt_out[AMOUNT_BUF_SIZE];
                    SET_AMOUNT_OUT(amt_out, host_token_ptr, account_field, balance_float);

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

                SET_AMOUNT_OUT(amt_out, host_token_ptr, account_field, 0);

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
                    COPY_BUF(last_host_id_buf, 0, host_addr_data, 0, 4);

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
            int64_t result = slot(SBUF(amount_buffer), amt_slot);
            if (result != AMOUNT_BUF_SIZE)
                rollback(SBUF("Evernode: Could not dump sfAmount"), 1);

            int is_evr;
            IS_EVR(is_evr, amount_buffer, hook_accid);

            // Get transaction hash(id).
            uint8_t txid[HASH_SIZE];
            int32_t txid_len = otxn_id(SBUF(txid), 0);
            if (txid_len < HASH_SIZE)
                rollback(SBUF("Evernode: transaction id missing!!!"), 1);

            // Host registration.
            int is_host_reg = 0;
            BUFFER_EQUAL_STR_GUARD(is_host_reg, type_ptr, type_len, HOST_REG, 1);
            if (is_host_reg)
            {
                // Currency should be EVR.
                if (!is_evr)
                    rollback(SBUF("Evernode: Currency should be EVR for host registration."), 1);

                BUFFER_EQUAL_STR_GUARD(is_host_reg, format_ptr, format_len, FORMAT_TEXT, 1);
                if (!is_host_reg)
                    rollback(SBUF("Evernode: Memo format should be text."), 50);

                // Take the host reg fee from config.
                int64_t conf_host_reg_fee;
                GET_FLOAT_CONF_VALUE(conf_host_reg_fee, DEF_HOST_REG_FEE_M, DEF_HOST_REG_FEE_E, CONF_HOST_REG_FEE, "Evernode: Could not set default state for host reg fee.");
                TRACEVAR(conf_host_reg_fee);

                if (float_compare(amt, conf_host_reg_fee, COMPARE_LESS) == 1)
                    rollback(SBUF("Evernode: Amount sent is less than the minimum fee for host registration."), 1);

                // Checking whether this host is already registered.
                HOST_ADDR_KEY(account_field);
                // <host_id(4)><hosting_token(3)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><audit_assigned_moment_start_idx(8)>
                // <auditor_addr(20)><rewarded_moment_start_idx(8)><accumulated_rewards(8)><locked_token_amont(8)><last_hearbeat_ledger_idx(8)>
                uint8_t host_addr[HOST_ADDR_VAL_SIZE];

                if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) != DOESNT_EXIST)
                    rollback(SBUF("Evernode: Host already registered."), 1);

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

                int64_t token_limit = float_sum(float_set(9, 1), float_negate(float_one())); // 999999999

                uint8_t amt_out[AMOUNT_BUF_SIZE];
                SET_AMOUNT_OUT(amt_out, data_ptr, account_field, token_limit);

                // Preparing trustline transaction.
                uint8_t txn_out[PREPARE_SIMPLE_TRUSTLINE_SIZE];
                PREPARE_SIMPLE_TRUSTLINE(txn_out, amt_out, fee);

                uint8_t emithash[HASH_SIZE];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting txn failed"), 1);

                trace(SBUF("emit hash: "), SBUF(emithash), 1);

                uint8_t host_count_buf[4];
                uint32_t host_count;
                GET_HOST_COUNT(host_count_buf, host_count);

                uint32_t host_id = host_count + 1;
                uint8_t host_id_arr[4];
                UINT32_TO_BUF(host_id_arr, host_id);
                HOST_ID_KEY(host_id_arr);
                if (state_set(SBUF(account_field), SBUF(STP_HOST_ID)) < 0)
                    rollback(SBUF("Evernode: Could not set state for host_id."), 1);

                CLEARBUF(host_addr);
                COPY_BUF(host_addr, 0, host_id_arr, 0, 4);
                COPY_BUF(host_addr, HOST_TOKEN_OFFSET, data_ptr, 0, 3);
                COPY_BUF(host_addr, HOST_COUNTRY_CODE_OFFSET, data_ptr, 4, COUNTRY_CODE_LEN);

                // Read instace details from the memo.
                // We cannot predict the lengths of the numarical values.
                // So we scan bytes and keep pointers and lengths to set in host address buffer.
                uint32_t section_number = 0;
                uint8_t *cpu_microsec_ptr, *ram_mb_ptr, *disk_mb_ptr, *description_ptr;
                uint32_t cpu_microsec_len = 0, ram_mb_len = 0, disk_mb_len = 0, description_len = 0;

                for (int i = 6; GUARD(data_len - 6), i < data_len; ++i)
                {
                    uint8_t *str_ptr = data_ptr + i;
                    // Colon means this is an end of the section.
                    // If so, we start reading the new section and reset the write index.
                    // Stop reading is an emty byte reached.
                    if (*str_ptr == ';')
                    {
                        section_number++;
                        continue;
                    }
                    else if (*str_ptr == 0)
                        break;

                    if (section_number == 1)
                    {
                        if (cpu_microsec_len == 0)
                            cpu_microsec_ptr = str_ptr;
                        cpu_microsec_len++;
                    }
                    else if (section_number == 2)
                    {
                        if (ram_mb_len == 0)
                            ram_mb_ptr = str_ptr;
                        ram_mb_len++;
                    }
                    else if (section_number == 3)
                    {
                        if (disk_mb_len == 0)
                            disk_mb_ptr = str_ptr;
                        disk_mb_len++;
                    }
                    else if (section_number == 4)
                    {
                        if (description_len == 0)
                            description_ptr = str_ptr;
                        description_len++;
                    }
                }

                // Take the unsigned int values.
                uint32_t cpu_microsec, ram_mb, disk_mb;
                STR_TO_UINT(cpu_microsec, cpu_microsec_ptr, cpu_microsec_len);
                STR_TO_UINT(ram_mb, ram_mb_ptr, ram_mb_len);
                STR_TO_UINT(disk_mb, disk_mb_ptr, disk_mb_len);

                // Populate the values to the buffer.
                UINT32_TO_BUF(&host_addr[HOST_CPU_MICROSEC_OFFSET], cpu_microsec);
                UINT32_TO_BUF(&host_addr[HOST_RAM_MB_OFFSET], ram_mb);
                UINT32_TO_BUF(&host_addr[HOST_DISK_MB_OFFSET], disk_mb);
                CLEAR_BUF(host_addr, HOST_RESERVED_OFFSET, 8);
                COPY_BUF(host_addr, HOST_DESCRIPTION_OFFSET, description_ptr, 0, description_len);
                if (description_len < DESCRIPTION_LEN)
                    CLEAR_BUF(host_addr, HOST_DESCRIPTION_OFFSET + description_len, DESCRIPTION_LEN - description_len);

                if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                    rollback(SBUF("Evernode: Could not set state for host_addr."), 1);

                host_count += 1;
                UINT32_TO_BUF(host_count_buf, host_count);
                if (state_set(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) < 0)
                    rollback(SBUF("Evernode: Could not set default state for host count."), 1);

                accept(SBUF("Host registration successful."), 0);
            }

            // Redeem request.
            int is_redeem_req = 0;
            BUFFER_EQUAL_STR_GUARD(is_redeem_req, type_ptr, type_len, REDEEM, 1);
            if (is_redeem_req)
            {
                if (is_evr)
                    rollback(SBUF("Evernode: Currency cannot be EVR for redeem request."), 1);

                BUFFER_EQUAL_STR_GUARD(is_redeem_req, format_ptr, format_len, FORMAT_BASE64, 1);
                if (!is_redeem_req)
                    rollback(SBUF("Evernode: Memo format should be base64 for redeem."), 50);
                // Checking whether this host is registered.
                uint8_t *issuer_ptr = &amount_buffer[28];
                HOST_ADDR_KEY(issuer_ptr);
                // <host_id(4)><hosting_token(3)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><audit_assigned_moment_start_idx(8)>
                // <auditor_addr(20)><rewarded_moment_start_idx(8)><accumulated_rewards(8)><locked_token_amont(8)><last_hearbeat_ledger_idx(8)>
                uint8_t host_addr[HOST_ADDR_VAL_SIZE];

                if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: Host is not registered."), 1);

                // Checking whether host is active.
                IS_HOST_ACTIVE(is_redeem_req, host_addr, cur_ledger_seq);
                if (!is_redeem_req)
                    rollback(SBUF("Evernode: Requested host is inactive."), 1);

                // Checking whether transaction is with host tokens.
                uint8_t *host_token_ptr = &host_addr[HOST_TOKEN_OFFSET];
                uint8_t hosting_token[20] = GET_TOKEN_CURRENCY(host_token_ptr);
                uint8_t is_hosting_token = 0;
                BUFFER_EQUAL_GUARD(is_hosting_token, hosting_token, 20, &amount_buffer[8], 20, 1);
                if (!is_hosting_token)
                    rollback(SBUF("Evernode: Currency should be in hosting tokens to redeem."), 1);

                // Take the minimum redeem amount from the config.
                uint16_t conf_min_redeem;
                GET_CONF_VALUE(conf_min_redeem, DEF_MIN_REDEEM, CONF_MIN_REDEEM, "Evernode: Could not set default state for min redeem.");
                TRACEVAR(conf_min_redeem);

                if (float_compare(amt, float_set(0, conf_min_redeem), COMPARE_LESS) == 1)
                    rollback(SBUF("Evernode: Amount sent is less than the minimum redeem amount."), 1);

                // Prepare state value.
                uint8_t redeem_op[REDEEM_STATE_VAL_SIZE]; // <hosting_token(3)><amount(8)><host_addr(20)><lcl_index(8)><user_addr(20)>

                // Set the host token.
                COPY_BUF(redeem_op, 0, amount_buffer, 20, 3)

                // Set the amount.
                uint8_t amount_buf[8];
                INT64_TO_BUF(amount_buf, amt);
                COPY_BUF(redeem_op, 3, amount_buf, 0, 8);

                // Set the issuer.
                COPY_BUF(redeem_op, 11, issuer_ptr, 0, 20);

                // Set the ledger.
                INT64_TO_BUF(&redeem_op[31], cur_ledger_seq);

                // Set the user address.
                COPY_BUF(redeem_op, 39, account_field, 0, 20);

                // Forward redeem to the host
                etxn_reserve(1);

                uint8_t origin_data[REDEEM_ORIGIN_DATA_LEN];
                // Set the user address.
                COPY_BUF(origin_data, 0, account_field, 0, 20);
                // Set the amount.
                COPY_BUF(origin_data, 20, amount_buf, 0, 8);
                // Set the host token.
                COPY_BUF(origin_data, 28, amount_buffer, 20, 3);
                // Set the redeem tx hash.
                COPY_BUF(origin_data, 31, txid, 0, 32);

                uint8_t redeem_data[data_len];
                COPY_BUF(redeem_data, 0, data_ptr, 0, data_len);

                int64_t fee = etxn_fee_base(PREPARE_PAYMENT_REDEEM_SIZE(sizeof(redeem_data), sizeof(origin_data)));

                uint8_t txn_out[PREPARE_PAYMENT_REDEEM_SIZE(sizeof(redeem_data), sizeof(origin_data))];
                PREPARE_PAYMENT_REDEEM_M(txn_out, MIN_DROPS, fee, issuer_ptr, redeem_data, sizeof(redeem_data), origin_data, sizeof(origin_data), 10);

                uint8_t emithash[HASH_SIZE];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting txn failed"), 1);
                trace(SBUF("emit hash: "), SBUF(emithash), 1);

                // Set state key with transaction hash(id).
                REDEEM_OP_KEY(txid);
                if (state_set(SBUF(redeem_op), SBUF(STP_REDEEM_OP)) < 0)
                    rollback(SBUF("Evernode: Could not set state for redeem_op."), 1);

                // Increasing the locked amount. since host receive tokens.
                uint8_t *locked_amount_ptr = &host_addr[HOST_LOCKED_TOKEN_AMT_OFFSET];
                int64_t cur_locked_amount = INT64_FROM_BUF(locked_amount_ptr);
                cur_locked_amount = float_sum(cur_locked_amount, amt);

                INT64_TO_BUF(locked_amount_ptr, cur_locked_amount);
                if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                    rollback(SBUF("Evernode: Could not set state for the locked amount."), 1);

                accept(SBUF("Redeem request successful."), 0);
            }

            // Host recharge.
            int is_recharge = 0;
            BUFFER_EQUAL_STR_GUARD(is_recharge, type_ptr, type_len, RECHARGE, 1);
            if (is_recharge)
            {
                if (is_evr)
                    rollback(SBUF("Evernode: Currency cannot be EVR for recharge."), 1);

                // Checking whether this host is registered.
                HOST_ADDR_KEY(account_field);
                // <host_id(4)><hosting_token(3)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><audit_assigned_moment_start_idx(8)>
                // <auditor_addr(20)><rewarded_moment_start_idx(8)><accumulated_rewards(8)><locked_token_amont(8)><last_hearbeat_ledger_idx(8)>
                uint8_t host_addr[HOST_ADDR_VAL_SIZE];

                if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: This host is not registered to perform recharge."), 1);

                // Checking whether this currency is issued by host.
                uint8_t *issuer_ptr = &amount_buffer[28];
                BUFFER_EQUAL_GUARD(is_recharge, issuer_ptr, 20, account_field, 20, 1);
                if (!is_recharge)
                    rollback(SBUF("Evernode: Currency should be issued by host to recharge."), 1);

                // Checking whether transaction is with host tokens
                uint8_t *host_token_ptr = &host_addr[HOST_TOKEN_OFFSET];
                uint8_t hosting_token[20] = GET_TOKEN_CURRENCY(host_token_ptr);
                BUFFER_EQUAL_GUARD(is_recharge, hosting_token, 20, &amount_buffer[8], 20, 1);
                if (!is_recharge)
                    rollback(SBUF("Evernode: Currency should be in hosting tokens to recharge."), 1);

                // Take the minimum redeem amount from the config.
                uint16_t conf_min_redeem;
                GET_CONF_VALUE(conf_min_redeem, DEF_MIN_REDEEM, CONF_MIN_REDEEM, "Evernode: Could not set default state for min redeem.");
                TRACEVAR(conf_min_redeem);

                int64_t min_redeem_float = float_set(0, conf_min_redeem);
                if (float_compare(amt, min_redeem_float, COMPARE_LESS) == 1)
                    rollback(SBUF("Evernode: Recharge amount is less than the minimum redeem amount."), 1);

                // Take the host heartbeat frequency from the config.
                uint16_t conf_host_heartbeat_freq;
                GET_CONF_VALUE(conf_host_heartbeat_freq, DEF_HOST_HEARTBEAT_FREQ, CONF_HOST_HEARTBEAT_FREQ, "Evernode: Could not set default state for host heartbeat freq.");
                TRACEVAR(conf_host_heartbeat_freq);

                // Take the current locked amount.
                uint8_t *locked_amount_ptr = &host_addr[HOST_LOCKED_TOKEN_AMT_OFFSET];
                int64_t cur_locked_amount = INT64_FROM_BUF(locked_amount_ptr);
                // Take the host heartbeat frequency.
                int64_t host_heartbeat_freq_float = float_set(0, conf_host_heartbeat_freq);
                // Calculate minimum required amount.
                int64_t required_amount = float_sum(cur_locked_amount, float_multiply(min_redeem_float, float_sum(host_heartbeat_freq_float, float_one())));
                // Calculate excess amount, In this case if received amount is MIN_REDEEM resulting value would be negative.
                // In the next step we add current token balane to this amount then we get the actual excess amount.
                int64_t excess_amount = float_sum(amt, float_negate(required_amount));

                // Taking the current token balance.
                uint8_t keylet[34];
                if (util_keylet(SBUF(keylet), KEYLET_LINE, SBUF(hook_accid), SBUF(account_field), SBUF(hosting_token)) != 34)
                    rollback(SBUF("Evernode: Internal error, could not generate keylet for host recharge"), 1);

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

                // Add the balance only if positive,
                if (balance_float < 0 && !IS_FLOAT_ZERO(balance_float))
                    rollback(SBUF("Evernode: Could not parse user trustline balance."), 1);
                else if (!IS_FLOAT_ZERO(balance_float))                                    // If we have positive balance.
                    excess_amount = float_sum(excess_amount, float_negate(balance_float)); // It gives negative value of the balance. Need to read and experiment more.

                // Float arithmatic sometimes gives -29 for 0, So we manually override float to 0.
                if (IS_FLOAT_ZERO(excess_amount))
                    excess_amount = 0;

                // If excess amount(amount we have + current recharge amount - max amount) < 0, hook does not have enough hosting tokens.
                // If excess amount > 0, we refund them to the host.
                if (float_compare(excess_amount, 0, COMPARE_LESS) == 1)
                    rollback(SBUF("Evernode: Recharge amount is less than minimum required hosting token amount."), 1);
                if (float_compare(excess_amount, 0, COMPARE_GREATER) == 1)
                {
                    // Send hosting tokens to the host.
                    etxn_reserve(1);

                    int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                    // Prepare currency.
                    uint8_t amt_out[AMOUNT_BUF_SIZE];
                    SET_AMOUNT_OUT(amt_out, host_token_ptr, account_field, excess_amount);

                    // Create the outgoing hosting token txn.
                    uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                    PREPARE_PAYMENT_SIMPLE_TRUSTLINE(txn_out, amt_out, fee, account_field, 0, 0);

                    uint8_t emithash[HASH_SIZE];
                    if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting txn failed"), 1);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);
                }

                // Set the last heartbeat ledger index of the host.
                INT64_TO_BUF(&host_addr[HOST_HEARTBEAT_LEDGER_IDX_OFFSET], cur_ledger_seq);
                if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                    rollback(SBUF("Evernode: Could not set state for the heartbeat ledger idx."), 1);

                accept(SBUF("Host recharge successful."), 0);
            }
        }
    }

    accept(SBUF("Evernode: Transaction type not supported."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
