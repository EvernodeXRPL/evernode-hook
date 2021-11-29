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
            // Redeem response.
            int is_redeem_res = 0;
            BUFFER_EQUAL_STR_GUARD(is_redeem_res, type_ptr, type_len, REDEEM_REF, 1);
            if (is_redeem_res)
            {
                BUFFER_EQUAL_STR_GUARD(is_redeem_res, format_ptr, format_len, FORMAT_BINARY, 1);
                if (!is_redeem_res)
                    rollback(SBUF("Evernode: Redeem reference memo format should be binary."), 50);

                if (data_len != 64)
                    rollback(SBUF("Evernode: Invalid redeem reference."), 1);

                uint8_t redeem_ref[data_len];
                COPY_BUF(redeem_ref, 0, data_ptr, 0, data_len);

                // Redeem response has 2 memos, so check for the type in second memo.
                GET_MEMO(1, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len);

                uint8_t redeem_res[data_len];
                COPY_BUF(redeem_res, 0, data_ptr, 0, data_len);

                // Redeem response should contain redeemResp and format should be binary.
                BUFFER_EQUAL_STR_GUARD(is_redeem_res, type_ptr, type_len, REDEEM_RESP, 1);
                if (!is_redeem_res)
                    rollback(SBUF("Evernode: Redeem response does not have instance info."), 1);

                int is_format_binary = 0, is_format_json = 0;
                BUFFER_EQUAL_STR_GUARD(is_format_binary, format_ptr, format_len, FORMAT_BINARY, 1);
                if (!is_format_binary)
                    BUFFER_EQUAL_STR_GUARD(is_format_json, format_ptr, format_len, FORMAT_JSON, 1);

                if (!(is_format_binary || is_format_json))
                    rollback(SBUF("Evernode: Redeem response memo format should be either binary or text/json."), 50);

                // Check for state with key as redeemRef.
                uint8_t hash_ptr[HASH_SIZE];
                HEXSTR_TO_BYTES(hash_ptr, redeem_ref, sizeof(redeem_ref));
                REDEEM_OP_KEY(hash_ptr);

                uint8_t redeem_op[REDEEM_STATE_VAL_SIZE]; // <hosting_token(3)><amount(8)><host_addr(20)><lcl_index(8)><user_addr(20)>
                if (state(SBUF(redeem_op), SBUF(STP_REDEEM_OP)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: No redeem state for the redeem response."), 1);

                // If redeem succeed we have to prepare two transactions.
                if (is_format_binary)
                    etxn_reserve(2);
                else
                    etxn_reserve(1);

                // Forward redeem to the user
                uint8_t *useraddr_ptr = &redeem_op[39];

                int64_t fee = etxn_fee_base(PREPARE_PAYMENT_REDEEM_RESP_SIZE(sizeof(redeem_ref), sizeof(redeem_res), is_format_binary));

                uint8_t txn_out[PREPARE_PAYMENT_REDEEM_RESP_SIZE(sizeof(redeem_ref), sizeof(redeem_res), is_format_binary)];
                PREPARE_PAYMENT_REDEEM_RESP(txn_out, MIN_DROPS, fee, useraddr_ptr, redeem_ref, sizeof(redeem_ref), redeem_res, sizeof(redeem_res), is_format_binary);

                uint8_t emithash[HASH_SIZE];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting user txn failed"), 1);
                trace(SBUF("emit user hash: "), SBUF(emithash), 1);

                // Send hosting tokens to the host and clear the state only if there's no error.
                if (is_format_binary)
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
                BUFFER_EQUAL_STR_GUARD(is_refund_req, format_ptr, format_len, FORMAT_BINARY, 1);
                if (!is_refund_req)
                    rollback(SBUF("Evernode: Memo format should be binary in refund request."), 1);

                if (data_len != 64) // 64 bytes is the size of the hash in hex
                    rollback(SBUF("Evernode: Memo data should be 64 bytes in hex in refund request."), 1);

                uint8_t tx_hash_bytes[HASH_SIZE];
                HEXSTR_TO_BYTES(tx_hash_bytes, data_ptr, data_len);
                REDEEM_OP_KEY(tx_hash_bytes);

                uint8_t data_arr[REDEEM_STATE_VAL_SIZE]; // <hosting_token(3)><amount(8)><host_addr(20)><lcl_index(8)><user_addr(20)>
                if (state(SBUF(data_arr), SBUF(STP_REDEEM_OP)) < 0)
                    rollback(SBUF("Evernode: No redeem for this tx hash."), 1);

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
                int64_t fee = etxn_fee_base(PREPARE_PAYMENT_REFUND_SIZE);

                uint8_t *issuer_ptr = &data_arr[11];
                uint8_t *amount_ptr = &data_arr[3];
                int64_t token_amount = INT64_FROM_BUF(amount_ptr);

                uint8_t amt_out[AMOUNT_BUF_SIZE];
                SET_AMOUNT_OUT(amt_out, data_arr, issuer_ptr, token_amount);

                // Get transaction hash(id).
                uint8_t txid[HASH_SIZE];
                int32_t txid_len = otxn_id(SBUF(txid), 0);
                if (txid_len < HASH_SIZE)
                    rollback(SBUF("Evernode: transaction id missing!!!"), 1);

                // Finally create the outgoing txn.
                uint8_t txn_out[PREPARE_PAYMENT_REFUND_SIZE];
                PREPARE_PAYMENT_REFUND(txn_out, amt_out, fee, account_field, tx_hash_bytes, txid);

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
            int is_audit_req = 0;
            BUFFER_EQUAL_STR_GUARD(is_audit_req, type_ptr, type_len, AUDIT_REQ, 1);

            // Audit success response.
            int is_audit_resp = 0;
            BUFFER_EQUAL_STR_GUARD(is_audit_resp, type_ptr, type_len, AUDIT_SUCCESS, 1);

            if (is_audit_req || is_audit_resp)
            {
                // Common checks for both audit request and audit suceess response.

                // Audit request is only served if at least one host is registered.
                uint8_t host_count_buf[4];
                uint32_t host_count;
                GET_HOST_COUNT(host_count_buf, host_count);
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

                    // Calculate the host id using seed.
                    // Selecting a host to audit.
                    uint8_t *audit_pick_ptr = &moment_seed_buf[8];
                    trace(SBUF("Moment seed: "), audit_pick_ptr, HASH_SIZE, 1);

                    // If auditor id is not within (HASH_SIZE - 3) we are taking sha512 hashes.
                    uint32_t eligible_count = HASH_SIZE - 3;
                    uint32_t iterations = (auditor_id - 1) / eligible_count;
                    uint32_t host_id;
                    if (iterations > 0)
                    {
                        uint8_t seed_hash[HASH_SIZE] = {0};
                        for (int i = 0; GUARD(iterations), i < iterations; ++i)
                        {
                            if (util_sha512h(SBUF(seed_hash), audit_pick_ptr, HASH_SIZE) < 0)
                                rollback(SBUF("Evernode: Could not generate the sha512h hash of the moment seed."), 1);
                            audit_pick_ptr = &seed_hash;
                        }
                        trace(SBUF("Auditor host pick buffer: "), audit_pick_ptr, HASH_SIZE, 1);
                        uint32_t pick_index = (auditor_id - 1) % eligible_count;
                        host_id = (UINT32_FROM_BUF(audit_pick_ptr + pick_index) % host_count) + 1;
                    }
                    else
                    {
                        trace(SBUF("Auditor host pick buffer: "), audit_pick_ptr, HASH_SIZE, 1);
                        uint32_t pick_index = auditor_id - 1;
                        host_id = (UINT32_FROM_BUF(audit_pick_ptr + pick_index) % host_count) + 1;
                    }

                    // Take the host address.
                    uint8_t host_addr[20];
                    uint8_t host_id_arr[4];
                    UINT32_TO_BUF(host_id_arr, host_id);
                    HOST_ID_KEY(host_id_arr);
                    if (state(SBUF(host_addr), SBUF(STP_HOST_ID)) == DOESNT_EXIST)
                        rollback(SBUF("Evernode: Could not find a matching host for the id."), 1);

                    // Take the last audit assigned moment.
                    HOST_ADDR_KEY(host_addr);
                    // <host_id(4)><hosting_token(3)><reg_tx_hash(32)><instance_size(60)><location(10)><audit_assigned_moment_start_idx(8)><rewarded_moment_start_idx(8)>
                    uint8_t host_addr_buf[HOST_ADDR_VAL_SIZE];
                    if (state(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                        rollback(SBUF("Evernode: Host is not registered."), 1);

                    uint8_t *host_token_ptr = &host_addr_buf[4];

                    // If host is already assigned for audit within this moment we won't reward again.
                    if (UINT64_FROM_BUF(&host_addr_buf[HOST_AUDIT_INFO_OFFSET]) == cur_moment_start_idx)
                        rollback(SBUF("Evernode: Picked host is already assigned for audit within this moment."), 1);

                    trace(SBUF("Hosting token"), host_token_ptr, 3, 1);

                    // Take the minimum redeem amount from the config.
                    uint16_t conf_min_redeem;
                    GET_CONF_VALUE(conf_min_redeem, DEF_MIN_REDEEM, CONF_MIN_REDEEM, "Evernode: Could not set default state for min redeem.");
                    TRACEVAR(conf_min_redeem);

                    // Setup the outgoing txn.
                    // Reserving one transaction.
                    etxn_reserve(1);
                    int64_t fee = etxn_fee_base(PREPARE_AUDIT_CHECK_SIZE);

                    int64_t token_limit = float_set(0, conf_min_redeem);

                    uint8_t amt_out[AMOUNT_BUF_SIZE];
                    SET_AMOUNT_OUT(amt_out, host_token_ptr, host_addr, token_limit);

                    // Finally create the outgoing txn.
                    uint8_t txn_out[PREPARE_AUDIT_CHECK_SIZE];
                    uint8_t audit_data[70];
                    COPY_BUF(audit_data, 0, host_addr_buf, 39, 70)
                    PREPARE_AUDIT_CHECK(txn_out, amt_out, fee, account_field, audit_data, sizeof(audit_data));

                    uint8_t emithash[HASH_SIZE];
                    if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting hosting token check failed."), 1);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);

                    // Update the auditor state.
                    COPY_BUF(auditor_addr_buf, 4, moment_seed_buf, 0, 8);
                    COPY_BUF(auditor_addr_buf, 12, host_addr, 0, 20);
                    if (state_set(SBUF(auditor_addr_buf), SBUF(STP_AUDITOR_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not update state for auditor_addr."), 1);

                    // Update the host's audit assigned state.
                    COPY_BUF(host_addr_buf, HOST_AUDIT_INFO_OFFSET, moment_seed_buf, 0, 8);
                    if (state_set(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not update audit moment for host_addr."), 1);

                    accept(SBUF("Evernode: Audit request successful."), 0);
                }
                else if (is_audit_resp) // Audit success response.
                {
                    // If auditor assigned moment idx is not equal to currect moment start idx.
                    // No host is assigned to audit for this momen.
                    if (lst_moment_start_idx != cur_moment_start_idx)
                        rollback(SBUF("Evernode: No host is assigned to audit for this moment."), 1);

                    // Take the last reward moment of the assigned host.
                    HOST_ADDR_KEY(lst_host_addr_ptr);
                    // <host_id(4)><hosting_token(3)><reg_tx_hash(32)><instance_size(60)><location(10)><audit_assigned_moment_start_idx(8)><rewarded_moment_start_idx(8)>
                    uint8_t host_addr_buf[HOST_ADDR_VAL_SIZE];
                    if (state(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                        rollback(SBUF("Evernode: Host is not registered."), 1);

                    // If host is not assigned for audit in this moment we won't reward.
                    if (UINT64_FROM_BUF(&host_addr_buf[HOST_AUDIT_INFO_OFFSET]) != cur_moment_start_idx)
                        rollback(SBUF("Evernode: Picked host is not assigned for audit in moment."), 1);

                    // If host is already rewarded within this moment we won't reward again.
                    if (UINT64_FROM_BUF(&host_addr_buf[HOST_AUDIT_INFO_OFFSET + 8]) == cur_moment_start_idx)
                        rollback(SBUF("Evernode: The host is already rewarded within this moment."), 1);

                    // Take the host rewards per moment from the config.
                    int64_t conf_reward;
                    GET_FLOAT_CONF_VALUE(conf_reward, DEF_REWARD_M, DEF_REWARD_E, CONF_REWARD, "Evernode: Could not set default state for host reward.");
                    TRACEVAR(conf_reward);

                    // Reward the host.
                    // Reserving one transaction.
                    etxn_reserve(1);

                    // Forward hosting tokens to the host on success.
                    int64_t fee = etxn_fee_base(PREPARE_PAYMENT_REWARD_SIZE);

                    // Reward amount would be, total reward amount equally divided by registered host count.
                    int64_t reward_amount = float_divide(conf_reward, float_set(0, host_count));

                    uint8_t amt_out[AMOUNT_BUF_SIZE];
                    SET_AMOUNT_OUT(amt_out, EVR_TOKEN, hook_accid, reward_amount);

                    // Create the outgoing hosting token txn.
                    uint8_t txn_out[PREPARE_PAYMENT_REWARD_SIZE];
                    PREPARE_PAYMENT_REWARD(txn_out, amt_out, fee, lst_host_addr_ptr);

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

                    // Update the host's audit rewarded state.
                    uint8_t cur_moment_start_idx_buf[8];
                    UINT64_TO_BUF(cur_moment_start_idx_buf, cur_moment_start_idx);
                    COPY_BUF(host_addr_buf, HOST_AUDIT_INFO_OFFSET + 8, cur_moment_start_idx_buf, 0, 8);
                    if (state_set(SBUF(host_addr_buf), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not update audit moment for host_addr."), 1);

                    accept(SBUF("Evernode: Audit success response successful."), 0);
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
                // <host_id(4)><hosting_token(3)><reg_tx_hash(32)><instance_size(60)><location(10)><audit_assigned_moment_start_idx(8)><rewarded_moment_start_idx(8)>
                uint8_t host_addr_data[HOST_ADDR_VAL_SIZE];
                if (state(SBUF(host_addr_data), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: This host is not registered."), 1);

                uint8_t *host_token_ptr = &host_addr_data[4];

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
                if (balance_float < 0 && balance_float != -29)
                    rollback(SBUF("Evernode: Could not parse user trustline balance."), 1);

                uint8_t amt_out[AMOUNT_BUF_SIZE];
                if (balance_float != -29) // It gives -29 (EXPONENT_UNDERSIZED) when balance is zero. Need to read and experiment more.
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
                // <host_id(4)><hosting_token(3)><reg_tx_hash(32)><instance_size(60)><location(10)><audit_assigned_moment_start_idx(8)><rewarded_moment_start_idx(8)>
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
                COPY_BUF(host_addr, 4, data_ptr, 0, 3);
                COPY_BUF(host_addr, 7, txid, 0, txid_len);

                // Read instace details from the memo.
                // We cannot predict the lengths of instance size and locations.
                // So we scan bytes and populate the buffer.
                uint32_t section_number = 0;
                uint32_t write_idx = 0;
                for (int i = 3; GUARD(data_len - 3), i < data_len; ++i)
                {
                    uint8_t *str_ptr = data_ptr + i;
                    // Colon means this is an end of the section.
                    // If so, we start reading the new section and reset the write index.
                    // Stop reading is an emty byte reached.
                    if (*str_ptr == ';')
                    {
                        section_number++;
                        write_idx = 0;
                        continue;
                    }
                    else if (*str_ptr == 0)
                        break;

                    // Section 1 is instance size.
                    // Only read first 60 bytes.
                    if (section_number == 1 && write_idx < INSTANCE_SIZE_LEN)
                    {
                        host_addr[INSTANCE_INFO_OFFSET + write_idx] = *str_ptr;
                        write_idx++;
                    }
                    // Section 2 is location.
                    // Only read first 10 bytes.
                    else if (section_number == 2 && write_idx < LOCATION_LEN)
                    {
                        host_addr[INSTANCE_INFO_OFFSET + INSTANCE_SIZE_LEN + write_idx] = *str_ptr;
                        write_idx++;
                    }
                }

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

                BUFFER_EQUAL_STR_GUARD(is_redeem_req, format_ptr, format_len, FORMAT_BINARY, 1);
                if (!is_redeem_req)
                    rollback(SBUF("Evernode: Memo format should be binary."), 50);
                // Checking whether this host is registered.
                uint8_t *issuer_ptr = &amount_buffer[28];
                HOST_ADDR_KEY(issuer_ptr);
                // <host_id(4)><hosting_token(3)><reg_tx_hash(32)><instance_size(60)><location(10)><audit_assigned_moment_start_idx(8)><rewarded_moment_start_idx(8)>
                uint8_t host_addr[HOST_ADDR_VAL_SIZE];

                if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: Host is not registered."), 1);

                // Checking whether transaction is with host tokens
                uint8_t *host_token_ptr = &host_addr[4];
                uint8_t hosting_token[20] = GET_TOKEN_CURRENCY(host_token_ptr);
                uint8_t is_hosting_token = 0;
                BUFFER_EQUAL_GUARD(is_hosting_token, hosting_token, 20, &amount_buffer[8], 20, 20);
                if (!is_hosting_token)
                    rollback(SBUF("Evernode: Currency should be in hosting tokens to redeem."), 1);

                // Take the minimum redeem amount from the config.
                uint16_t conf_min_redeem;
                GET_CONF_VALUE(conf_min_redeem, DEF_MIN_REDEEM, CONF_MIN_REDEEM, "Evernode: Could not set default state for min redeem.");
                TRACEVAR(conf_min_redeem);

                if (float_compare(amt, float_set(0, conf_min_redeem), COMPARE_LESS) == 1)
                    rollback(SBUF("Evernode: Amount sent is less than the minimum registration fee."), 1);

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
                int64_t ledger = ledger_seq();
                uint8_t ledger_buf[8];
                INT64_TO_BUF(ledger_buf, ledger);
                COPY_BUF(redeem_op, 31, ledger_buf, 0, 8);

                // Set the user address.
                COPY_BUF(redeem_op, 39, account_field, 0, 20);

                // Set state key with transaction hash(id).
                REDEEM_OP_KEY(txid);
                if (state_set(SBUF(redeem_op), SBUF(STP_REDEEM_OP)) < 0)
                    rollback(SBUF("Evernode: Could not set state for redeem_op."), 1);

                // Forward redeem to the host
                etxn_reserve(1);

                uint8_t data[data_len];
                COPY_BUF(data, 0, data_ptr, 0, data_len);

                int64_t fee = etxn_fee_base(PREPARE_PAYMENT_REDEEM_SIZE(sizeof(data)));

                uint8_t txn_out[PREPARE_PAYMENT_REDEEM_SIZE(sizeof(data))];
                PREPARE_PAYMENT_REDEEM(txn_out, MIN_DROPS, fee, issuer_ptr, data, sizeof(data));

                uint8_t emithash[HASH_SIZE];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting txn failed"), 1);
                trace(SBUF("emit hash: "), SBUF(emithash), 1);

                accept(SBUF("Redeem request successful."), 0);
            }
        }
    }

    accept(SBUF("Evernode: Transaction type not supported."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
