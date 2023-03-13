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
    int64_t cur_ledger_seq = ledger_seq();
    /**
     * Calculate corresponding XRPL timestamp.
     * This calculation is based on the UNIX timestamp & XRPL timestamp relationship
     * https://xrpl-hooks.readme.io/reference/ledger_last_time#behaviour
     */
    int64_t cur_ledger_timestamp = ledger_last_time() + XRPL_TIMESTAMP_OFFSET;

    // <transition index><transition_moment><index_type>
    uint8_t moment_base_info[MOMENT_BASE_INFO_VAL_SIZE] = {0};
    int moment_base_info_state_res = state_foreign(moment_base_info, MOMENT_BASE_INFO_VAL_SIZE, SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF);
    if (moment_base_info_state_res < 0 && moment_base_info_state_res != DOESNT_EXIST)
        rollback(SBUF("Evernode: Could not get moment base info state."), 1);
    uint64_t moment_base_idx = UINT64_FROM_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);
    uint32_t prev_transition_moment = UINT32_FROM_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]);
    // If state does not exist, take the moment type from default constant.
    uint8_t cur_moment_type = (moment_base_info_state_res == DOESNT_EXIST) ? DEF_MOMENT_TYPE : moment_base_info[MOMENT_TYPE_OFFSET];
    uint64_t cur_idx = cur_moment_type == TIMESTAMP_MOMENT_TYPE ? cur_ledger_timestamp : cur_ledger_seq;

    // Take the moment size from config.
    uint8_t moment_size_buf[2];
    int64_t moment_size_res = state_foreign(SBUF(moment_size_buf), SBUF(CONF_MOMENT_SIZE), FOREIGN_REF);
    if (moment_size_res < 0 && moment_base_info_state_res != DOESNT_EXIST)
        rollback(SBUF("Evernode: Could not get moment size."), 1);
    uint16_t moment_size = UINT16_FROM_BUF(moment_size_buf);

    ///////////////////////////////////////////////////////////////
    /////// Moment transition related logic is handled here ///////

    // <transition_index(uint64_t)><moment_size(uint16_t)><index_type(uint8_t)>
    uint8_t moment_transition_info[MOMENT_TRANSIT_INFO_VAL_SIZE] = {0};
    int transition_state_res = state_foreign(moment_transition_info, MOMENT_TRANSIT_INFO_VAL_SIZE, SBUF(CONF_MOMENT_TRANSIT_INFO), FOREIGN_REF);
    if (transition_state_res < 0 && transition_state_res != DOESNT_EXIST)
        rollback(SBUF("Evernode: Error getting moment size transaction info state."), 1);

    if (transition_state_res >= 0)
    {
        // Begin : Moment size transition implementation.
        // If there is a transition, transition_idx specifies a index value to perform that.
        uint64_t transition_idx = UINT64_FROM_BUF(&moment_transition_info[TRANSIT_IDX_OFFSET]);
        if (transition_idx > 0 && cur_idx >= transition_idx)
        {
            uint8_t transit_moment_type = moment_transition_info[TRANSIT_MOMENT_TYPE_OFFSET];

            // Take the transition moment
            const uint32_t transition_moment = GET_MOMENT(transition_idx);

            // Add new moment size to the state.
            const uint8_t *moment_size_ptr = &moment_transition_info[TRANSIT_MOMENT_SIZE_OFFSET];
            if (state_foreign_set(moment_size_ptr, 2, SBUF(CONF_MOMENT_SIZE), FOREIGN_REF) < 0)
                rollback(SBUF("Evernode: Could not update the state for moment size."), 1);

            // Update the moment base info.
            UINT64_TO_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET], transition_idx);
            UINT32_TO_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET], transition_moment);
            moment_base_info[MOMENT_TYPE_OFFSET] = moment_transition_info[TRANSIT_MOMENT_TYPE_OFFSET];
            if (state_foreign_set(SBUF(moment_base_info), SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF) < 0)
                rollback(SBUF("Evernode: Could not set state for moment base info."), 1);

            // Assign the transition state values with zeros.
            CLEAR_MOMENT_TRANSIT_INFO(moment_transition_info);
            if (state_foreign_set(SBUF(moment_transition_info), SBUF(CONF_MOMENT_TRANSIT_INFO), FOREIGN_REF) < 0)
                rollback(SBUF("Evernode: Could not set state for moment transition info."), 1);

            moment_base_idx = UINT64_FROM_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);
            prev_transition_moment = UINT32_FROM_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]);
            cur_moment_type = moment_base_info[MOMENT_TYPE_OFFSET];
            cur_idx = cur_moment_type == TIMESTAMP_MOMENT_TYPE ? cur_ledger_timestamp : cur_ledger_seq;
            moment_size = UINT16_FROM_BUF(moment_size_ptr);
        }
        // End : Moment size transition implementation.
    }

    ///////////////////////////////////////////////////////////////

    int64_t txn_type = otxn_type();
    if (txn_type == ttPAYMENT)
    {
        // Getting the hook account id.
        unsigned char hook_accid[20];
        hook_account((uint32_t)hook_accid, 20);

        // Next fetch the sfAccount field from the originating transaction
        uint8_t account_field[ACCOUNT_ID_SIZE];
        int32_t account_field_len = otxn_field(SBUF(account_field), sfAccount);
        if (account_field_len < 20)
            rollback(SBUF("Evernode: sfAccount field is missing."), 1);

        // Get transaction hash(id).
        uint8_t txid[HASH_SIZE];
        const int32_t txid_len = otxn_id(SBUF(txid), 0);
        if (txid_len < HASH_SIZE)
            rollback(SBUF("Evernode: transaction id missing."), 1);

        // Accept any outgoing transactions without further processing.
        if (!BUFFER_EQUAL_20(hook_accid, account_field))
        {
            // Memos
            uint8_t memos[MAX_MEMO_SIZE];
            int64_t memos_len = otxn_field(SBUF(memos), sfMemos);

            if (memos_len)
            {
                uint8_t *memo_ptr, *type_ptr, *format_ptr, *data_ptr, *data_ptr1;
                uint32_t memo_len, type_len, format_len, data_len;
                int64_t memo_lookup = sto_subarray(memos, memos_len, 0);
                GET_MEMO(memo_lookup, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len);

                memo_lookup = sto_subarray(memos, memos_len, 1);
                if (memo_lookup > 0)
                {
                    uint8_t *memo_ptr1, *type_ptr1, *format_ptr1;
                    uint32_t memo_len1, type_len1, format_len1, data_len1;
                    GET_MEMO(memo_lookup, memos, memos_len, memo_ptr1, memo_len1, type_ptr1, type_len1, format_ptr1, format_len1, data_ptr1, data_len1);

                    if (!EQUAL_CANDIDATE_PROPOSE_REF(type_ptr1, type_len1))
                        rollback(SBUF("Evernode: Memo type is invalid."), 1);

                    if (!EQUAL_FORMAT_HEX(format_ptr1, format_len1))
                        rollback(SBUF("Evernode: Memo format should be hex."), 1);
                }

                // specifically we're interested in the amount sent
                int64_t oslot = otxn_slot(0);
                if (oslot < 0)
                    rollback(SBUF("Evernode: Could not slot originating txn."), 1);

                int64_t amt_slot = slot_subfield(oslot, sfAmount, 0);

                uint8_t op_type = OP_NONE;
                uint8_t source_is_foundation = 0;

                if (amt_slot < 0)
                    rollback(SBUF("Evernode: Could not slot otxn.sfAmount"), 1);

                int64_t is_xrp = slot_type(amt_slot, 1);
                if (is_xrp < 0)
                    rollback(SBUF("Evernode: Could not determine sent amount type"), 1);

                if (is_xrp)
                {
                    // Host initialization.
                    if (EQUAL_INITIALIZE(type_ptr, type_len))
                        op_type = OP_INITIALIZE;
                    // Change Governance Mode.
                    else if (EQUAL_CHANGE_GOVERNANCE_MODE(type_ptr, type_len))
                        op_type = OP_GOVERNANCE_MODE_CHANGE;
                    // Vote for a Hook Candidate.
                    else if (EQUAL_CANDIDATE_VOTE(type_ptr, type_len))
                        op_type = OP_VOTE;
                    // Invoke Governor on host heartbeat.
                    else if (EQUAL_CANDIDATE_STATUS_CHANGE(type_ptr, type_len))
                        op_type = OP_STATUS_CHANGE;
                    // Hook candidate withdraw request.
                    else if (EQUAL_CANDIDATE_WITHDRAW(type_ptr, type_len))
                        op_type = OP_WITHDRAW;
                    // Child hook update results.
                    else if (EQUAL_HOOK_UPDATE_RES(type_ptr, type_len))
                        op_type = OP_HOOK_UPDATE;
                }
                else // IOU payments.
                {
                    // Proposing a Hook Candidate.
                    if (EQUAL_CANDIDATE_PROPOSE(type_ptr, type_len))
                        op_type = OP_PROPOSE;
                    // Dud host report.
                    else if (EQUAL_DUD_HOST_REPORT(type_ptr, type_len))
                        op_type = OP_DUD_HOST_REPORT;
                }

                if (op_type != OP_NONE)
                {
                    // Memo format validation.
                    if (!EQUAL_FORMAT_HEX(format_ptr, format_len))
                        rollback(SBUF("Evernode: Memo format should be hex."), 1);

                    // <token_id(32)><country_code(2)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)><no_of_total_instances(4)><no_of_active_instances(4)>
                    // <last_heartbeat_index(8)><version(3)><registration_timestamp(8)><transfer_flag(1)><last_vote_candidate_idx(4)><support_vote_sent(1)>
                    uint8_t host_addr[HOST_ADDR_VAL_SIZE];
                    // <host_address(20)><cpu_model_name(40)><cpu_count(2)><cpu_speed(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)>
                    uint8_t token_id[TOKEN_ID_VAL_SIZE];
                    // <eligibility_period(uint32_t)><candidate_life_period<uint32_t)><candidate_election_period<uint32_t)><candidate_support_average<uint16_t)>
                    uint8_t governance_configuration[GOVERNANCE_CONFIGURATION_VAL_SIZE] = {0};
                    uint8_t issuer_accid[ACCOUNT_ID_SIZE] = {0};
                    uint8_t foundation_accid[ACCOUNT_ID_SIZE] = {0};
                    if (op_type == OP_PROPOSE || op_type == OP_VOTE || op_type == OP_STATUS_CHANGE || op_type == OP_WITHDRAW || op_type == OP_DUD_HOST_REPORT || op_type == OP_GOVERNANCE_MODE_CHANGE)
                    {
                        if (state_foreign(SBUF(governance_configuration), SBUF(CONF_GOVERNANCE_CONFIGURATION), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not get governance configuration state."), 1);

                        if (state_foreign(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not get issuer account id."), 1);

                        if (state_foreign(foundation_accid, ACCOUNT_ID_SIZE, SBUF(CONF_FOUNDATION_ADDR), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not get state for foundation account."), 1);

                        source_is_foundation = BUFFER_EQUAL_20(foundation_accid, account_field);
                    }

                    // Validation check for participants other than the foundation address
                    if ((op_type == OP_PROPOSE || op_type == OP_VOTE || op_type == DUD_HOST_REPORT) && !source_is_foundation)
                    {
                        const uint32_t min_eligibility_period = UINT32_FROM_BUF(&governance_configuration[ELIGIBILITY_PERIOD_OFFSET]);

                        HOST_ADDR_KEY(account_field);
                        // Check for registration entry.
                        if (state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) == DOESNT_EXIST)
                            rollback(SBUF("Evernode: This host is not registered."), 1);

                        uint8_t eligible_for_governance = 0;
                        uint8_t do_rollback = 1;
                        VALIDATE_GOVERNANCE_ELIGIBILITY(host_addr, cur_ledger_timestamp, min_eligibility_period, eligible_for_governance, do_rollback);
                    }

                    // <governance_mode(1)><last_candidate_idx(4)><voter_base_count(4)><voter_base_count_changed_timestamp(8)><foundation_last_voted_candidate_idx(4)><elected_proposal_unique_id(32)>
                    // <proposal_elected_timestamp(8)><updated_hook_count(1)><foundation_vote_flag(1)>
                    uint8_t governance_info[GOVERNANCE_INFO_VAL_SIZE];
                    if (op_type == OP_STATUS_CHANGE || op_type == OP_HOOK_UPDATE || op_type == OP_GOVERNANCE_MODE_CHANGE || op_type == OP_VOTE || op_type == OP_PROPOSE || op_type == OP_DUD_HOST_REPORT)
                    {
                        if (state_foreign(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) == DOESNT_EXIST)
                            rollback(SBUF("Evernode: Could not get state governance_game info."), 1);
                    }

                    uint8_t heartbeat_accid[ACCOUNT_ID_SIZE] = {0};
                    uint8_t registry_accid[ACCOUNT_ID_SIZE] = {0};
                    // <owner_address(20)><candidate_idx(4)><short_name(20)><created_timestamp(8)><proposal_fee(8)><positive_vote_count(4)>
                    // <last_vote_timestamp(8)><status(1)><status_change_timestamp(8)><foundation_vote_status(1)>
                    uint8_t candidate_id[CANDIDATE_ID_VAL_SIZE];
                    // <GOVERNOR_HASH(32)><REGISTRY_HASH(32)><HEARTBEAT_HASH(32)>
                    uint8_t candidate_owner[CANDIDATE_OWNER_VAL_SIZE];
                    if (op_type == OP_STATUS_CHANGE || op_type == OP_HOOK_UPDATE || op_type == OP_VOTE || op_type == OP_WITHDRAW)
                    {
                        if (state_foreign(SBUF(heartbeat_accid), SBUF(CONF_HEARTBEAT_ADDR), FOREIGN_REF) < 0 || state_foreign(SBUF(registry_accid), SBUF(CONF_REGISTRY_ADDR), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not get heartbeat or registry account id."), 1);

                        CANDIDATE_ID_KEY(data_ptr);
                        if (state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Error getting a candidate for the given id."), 1);

                        // As first 20 bytes of "candidate_id" represents owner address.
                        CANDIDATE_OWNER_KEY(candidate_id);
                    }

                    uint8_t candidate_type;
                    int vote_status;
                    if (op_type == OP_VOTE || op_type == OP_STATUS_CHANGE)
                    {
                        candidate_type = CANDIDATE_TYPE(data_ptr);
                        if (candidate_type == 0)
                            rollback(SBUF("Evernode: Invalid candidate type."), 1);
                        vote_status = CANDIDATE_REJECTED;
                    }

                    // <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)>
                    uint8_t reward_info[REWARD_INFO_VAL_SIZE];
                    // <epoch_count(uint8_t)><first_epoch_reward_quota(uint32_t)><epoch_reward_amount(uint32_t)><reward_start_moment(uint32_t)>
                    uint8_t reward_configuration[REWARD_CONFIGURATION_VAL_SIZE];
                    if (op_type == OP_PROPOSE || op_type == OP_DUD_HOST_REPORT || op_type == OP_STATUS_CHANGE)
                    {
                        if (state_foreign(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO), FOREIGN_REF) < 0 ||
                            state_foreign(reward_configuration, REWARD_CONFIGURATION_VAL_SIZE, SBUF(CONF_REWARD_CONFIGURATION), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not get reward configuration or reward info states."), 1);
                    }

                    int64_t proposal_fee;
                    if (op_type == OP_PROPOSE || op_type == OP_DUD_HOST_REPORT)
                    {
                        // BEGIN : Proposal Fee validations.
                        uint8_t amount_buffer[AMOUNT_BUF_SIZE];
                        const int64_t result = slot(SBUF(amount_buffer), amt_slot);
                        if (result != AMOUNT_BUF_SIZE)
                            rollback(SBUF("Evernode: Could not dump sfAmount"), 1);

                        const int64_t float_amt = slot_float(amt_slot);
                        if (float_amt < 0)
                            rollback(SBUF("Evernode: Could not parse amount."), 1);

                        // Currency should be EVR.
                        if (!IS_EVR(amount_buffer, issuer_accid))
                            rollback(SBUF("Evernode: Currency should be EVR for candidate proposal."), 1);

                        const uint8_t epoch_count = reward_configuration[EPOCH_COUNT_OFFSET];
                        const uint32_t first_epoch_reward_quota = UINT32_FROM_BUF(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET]);

                        const uint8_t epoch = reward_info[EPOCH_OFFSET];
                        uint32_t reward_quota;
                        GET_EPOCH_REWARD_QUOTA(epoch, first_epoch_reward_quota, reward_quota);

                        // Proposal fee for dud host remove would be 25% of reward quota.
                        proposal_fee = float_set(0, reward_quota);
                        proposal_fee = op_type == OP_PROPOSE ? proposal_fee : float_multiply(proposal_fee, float_set(-2, 25));

                        if (float_compare(float_amt, proposal_fee, COMPARE_LESS) == 1)
                            rollback(SBUF("Evernode: Proposal fee amount is less than the minimum fee."), 1);

                        proposal_fee = float_amt;
                        // END : Proposal Fee validations.
                    }

                    if (op_type == OP_INITIALIZE)
                    {
                        uint8_t initializer_accid[ACCOUNT_ID_SIZE];
                        const int initializer_accid_len = util_accid(SBUF(initializer_accid), HOOK_INITIALIZER_ADDR, 35);
                        if (initializer_accid_len < ACCOUNT_ID_SIZE)
                            rollback(SBUF("Evernode: Could not convert initializer account id."), 1);

                        // We accept only the init transaction from hook initializer account
                        if (!BUFFER_EQUAL_20(initializer_accid, account_field))
                            rollback(SBUF("Evernode: Only initializer is allowed to initialize states."), 1);

                        if (data_len != (4 * ACCOUNT_ID_SIZE))
                            rollback(SBUF("Evernode: Memo data should contain foundation cold wallet, registry heartbeat hook and issuer addresses."), 1);

                        uint8_t *issuer_ptr = data_ptr;
                        uint8_t *foundation_ptr = data_ptr + ACCOUNT_ID_SIZE;
                        uint8_t *registry_hook_ptr = data_ptr + (2 * ACCOUNT_ID_SIZE);
                        uint8_t *heartbeat_hook_ptr = data_ptr + (3 * ACCOUNT_ID_SIZE);

                        // First check if the states are already initialized by checking one state key for existence.
                        int already_initalized = 0; // For Beta test purposes
                        uint8_t host_count_buf[8];
                        if (state_foreign(SBUF(host_count_buf), SBUF(STK_HOST_COUNT), FOREIGN_REF) != DOESNT_EXIST)
                        {
                            already_initalized = 1;
                            rollback(SBUF("Evernode: State is already initialized."), 1);
                        }

                        const uint64_t zero = 0;
                        // Initialize the state.
                        if (!already_initalized)
                        {
                            SET_UINT_STATE_VALUE(zero, STK_HOST_COUNT, "Evernode: Could not initialize state for host count.");

                            uint8_t moment_base_info_buf[MOMENT_BASE_INFO_VAL_SIZE] = {0};
                            moment_base_info_buf[MOMENT_TYPE_OFFSET] = DEF_MOMENT_TYPE;
                            if (state_foreign_set(SBUF(moment_base_info_buf), SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not initialize state for moment base info."), 1);

                            SET_UINT_STATE_VALUE(DEF_MOMENT_SIZE, CONF_MOMENT_SIZE, "Evernode: Could not initialize state for moment size.");
                            moment_size = DEF_MOMENT_SIZE;
                            SET_UINT_STATE_VALUE(DEF_HOST_REG_FEE, STK_HOST_REG_FEE, "Evernode: Could not initialize state for reg fee.");
                            SET_UINT_STATE_VALUE(DEF_MAX_REG, STK_MAX_REG, "Evernode: Could not initialize state for maximum registrants.");

                            if (state_foreign_set(issuer_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_ISSUER_ADDR), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for issuer account."), 1);

                            if (state_foreign_set(foundation_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_FOUNDATION_ADDR), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for foundation account."), 1);

                            if (state_foreign_set(heartbeat_hook_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_HEARTBEAT_ADDR), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for heartbeat hook account."), 1);

                            if (state_foreign_set(registry_hook_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_REGISTRY_ADDR), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for registry hook account."), 1);
                        }

                        if (state_foreign(SBUF(reward_info), SBUF(STK_REWARD_INFO), FOREIGN_REF) == DOESNT_EXIST)
                        {
                            const uint32_t cur_moment = GET_MOMENT(cur_idx);
                            reward_info[EPOCH_OFFSET] = 1;
                            UINT32_TO_BUF(&reward_info[SAVED_MOMENT_OFFSET], cur_moment);
                            UINT32_TO_BUF(&reward_info[PREV_MOMENT_ACTIVE_HOST_COUNT_OFFSET], zero);
                            UINT32_TO_BUF(&reward_info[CUR_MOMENT_ACTIVE_HOST_COUNT_OFFSET], zero);
                            INT64_TO_BUF(&reward_info[EPOCH_POOL_OFFSET], float_set(0, DEF_EPOCH_REWARD_AMOUNT));
                            if (state_foreign_set(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for reward info."), 1);
                        }

                        SET_UINT_STATE_VALUE(DEF_MINT_LIMIT, CONF_MINT_LIMIT, "Evernode: Could not initialize state for mint limit.");
                        SET_UINT_STATE_VALUE(DEF_FIXED_REG_FEE, CONF_FIXED_REG_FEE, "Evernode: Could not initialize state for fixed reg fee.");
                        SET_UINT_STATE_VALUE(DEF_HOST_HEARTBEAT_FREQ, CONF_HOST_HEARTBEAT_FREQ, "Evernode: Could not initialize state for heartbeat frequency.");
                        SET_UINT_STATE_VALUE(DEF_LEASE_ACQUIRE_WINDOW, CONF_LEASE_ACQUIRE_WINDOW, "Evernode: Could not initialize state for lease acquire window.");
                        SET_UINT_STATE_VALUE(DEF_MAX_TOLERABLE_DOWNTIME, CONF_MAX_TOLERABLE_DOWNTIME, "Evernode: Could not initialize maximum tolerable downtime.");
                        SET_UINT_STATE_VALUE(DEF_EMIT_FEE_THRESHOLD, CONF_MAX_EMIT_TRX_FEE, "Evernode: Could not initialize maximum transaction fee for an emission.");

                        // <epoch_count(uint8_t)><first_epoch_reward_quota(uint32_t)><epoch_reward_amount(uint32_t)><reward_start_moment(uint32_t)>;
                        uint8_t reward_configuration[REWARD_CONFIGURATION_VAL_SIZE];
                        reward_configuration[EPOCH_COUNT_OFFSET] = DEF_EPOCH_COUNT;
                        UINT32_TO_BUF(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET], DEF_FIRST_EPOCH_REWARD_QUOTA);
                        UINT32_TO_BUF(&reward_configuration[EPOCH_REWARD_AMOUNT_OFFSET], DEF_EPOCH_REWARD_AMOUNT);
                        UINT32_TO_BUF(&reward_configuration[REWARD_START_MOMENT_OFFSET], DEF_REWARD_START_MOMENT);
                        if (state_foreign_set(reward_configuration, REWARD_CONFIGURATION_VAL_SIZE, SBUF(CONF_REWARD_CONFIGURATION), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not set state for reward configuration."), 1);

                        UINT32_TO_BUF(&governance_configuration[ELIGIBILITY_PERIOD_OFFSET], DEF_GOVERNANCE_ELIGIBILITY_PERIOD);
                        UINT32_TO_BUF(&governance_configuration[CANDIDATE_LIFE_PERIOD_OFFSET], DEF_CANDIDATE_LIFE_PERIOD);
                        UINT32_TO_BUF(&governance_configuration[CANDIDATE_ELECTION_PERIOD_OFFSET], DEF_CANDIDATE_ELECTION_PERIOD);
                        UINT16_TO_BUF(&governance_configuration[CANDIDATE_SUPPORT_AVERAGE_OFFSET], DEF_CANDIDATE_SUPPORT_AVERAGE);
                        if (state_foreign_set(SBUF(governance_configuration), SBUF(CONF_GOVERNANCE_CONFIGURATION), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not set state for governance configuration."), 1);

                        if (state_foreign(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) == DOESNT_EXIST)
                        {
                            governance_info[EPOCH_OFFSET] = PILOTED;
                            if (state_foreign_set(governance_info, GOVERNANCE_INFO_VAL_SIZE, SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for governance info."), 1);
                        }

                        ///////////////////////////////////////////////////////////////
                        // Begin : Moment size transition implementation.
                        // Do the moment size transition. If new moment size is specified.
                        // Set Moment transition info with the configured value;
                        if (NEW_MOMENT_SIZE > 0 && moment_size != NEW_MOMENT_SIZE)
                        {
                            if (!IS_MOMENT_TRANSIT_INFO_EMPTY(moment_transition_info))
                                rollback(SBUF("Evernode: There is an already scheduled moment size transition."), 1);

                            const uint64_t next_moment_start_idx = GET_NEXT_MOMENT_START_INDEX(cur_idx);

                            UINT64_TO_BUF(moment_transition_info + TRANSIT_IDX_OFFSET, next_moment_start_idx);
                            UINT16_TO_BUF(moment_transition_info + TRANSIT_MOMENT_SIZE_OFFSET, NEW_MOMENT_SIZE);
                            moment_transition_info[TRANSIT_MOMENT_TYPE_OFFSET] = NEW_MOMENT_TYPE;

                            if (state_foreign_set(moment_transition_info, MOMENT_TRANSIT_INFO_VAL_SIZE, SBUF(CONF_MOMENT_TRANSIT_INFO), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for moment transition info."), 1);
                        }
                        // End : Moment size transition implementation.
                        ///////////////////////////////////////////////////////////////

                        accept(SBUF("Evernode: Initialization successful."), 0);
                    }
                    else if (op_type == OP_PROPOSE)
                    {
                        int hooks_exists = 0;
                        IS_HOOKS_VALID((data_ptr1 + CANDIDATE_PROPOSE_KEYLETS_MEMO_OFFSET), hooks_exists);
                        if (hooks_exists == 0)
                            rollback(SBUF("Evernode: Provided hooks are not valid."), 1);

                        CANDIDATE_OWNER_KEY(account_field);
                        if (state_foreign(SBUF(candidate_owner), SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) != DOESNT_EXIST)
                            rollback(SBUF("Evernode: This owner has already created a proposal."), 1);

                        uint8_t unique_id[HASH_SIZE] = {0};
                        GET_NEW_HOOK_CANDIDATE_ID(data_ptr, data_len, unique_id);

                        if (!BUFFER_EQUAL_32((data_ptr1 + CANDIDATE_PROPOSE_UNIQUE_ID_MEMO_OFFSET), unique_id))
                            rollback(SBUF("Evernode: Generated candidate hash is not matched with provided."), 1);

                        CANDIDATE_ID_KEY(unique_id);
                        if (state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) != DOESNT_EXIST)
                            rollback(SBUF("Evernode: This candidate is already proposed"), 1);

                        const uint32_t candidate_idx = (UINT32_FROM_BUF(&governance_info[LAST_CANDIDATE_IDX_OFFSET]) + 1);

                        COPY_20BYTES((candidate_id + CANDIDATE_OWNER_ADDRESS_OFFSET), account_field);
                        UINT32_TO_BUF(&candidate_id[CANDIDATE_IDX_OFFSET], candidate_idx);
                        COPY_20BYTES((candidate_id + CANDIDATE_SHORT_NAME_OFFSET), (data_ptr1 + CANDIDATE_PROPOSE_SHORT_NAME_MEMO_OFFSET));
                        UINT64_TO_BUF(&candidate_id[CANDIDATE_CREATED_TIMESTAMP_OFFSET], cur_ledger_timestamp);
                        INT64_TO_BUF(&candidate_id[CANDIDATE_PROPOSAL_FEE_OFFSET], proposal_fee);
                        UINT32_TO_BUF(&candidate_id[CANDIDATE_POSITIVE_VOTE_COUNT_OFFSET], 0);
                        UINT64_TO_BUF(&candidate_id[CANDIDATE_LAST_VOTE_TIMESTAMP_OFFSET], 0);
                        candidate_id[CANDIDATE_STATUS_OFFSET] = CANDIDATE_REJECTED;
                        UINT64_TO_BUF(&candidate_id[CANDIDATE_STATUS_CHANGE_TIMESTAMP_OFFSET], cur_ledger_timestamp);
                        candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET] = CANDIDATE_REJECTED;

                        // Write to states.
                        COPY_CANDIDATE_HASHES(candidate_owner, data_ptr);
                        if (state_foreign_set(SBUF(candidate_owner), SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) < 0 || state_foreign_set(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not set state for candidate."), 1);

                        // Update the new candidate index.
                        UINT32_TO_BUF(&governance_info[LAST_CANDIDATE_IDX_OFFSET], candidate_idx);
                        if (state_foreign_set(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not set governance info state."), 1);

                        accept(SBUF("Evernode: Successfully accepted Hook candidate proposal."), 0);
                    }
                    else if (op_type == OP_VOTE)
                    {
                        if (!source_is_foundation)
                            rollback(SBUF("Evernode: Only foundation is allowed to vote in governance hook."), 1);

                        if (BUFFER_EQUAL_32((governance_info + ELECTED_PROPOSAL_UNIQUE_ID_OFFSET), data_ptr))
                            rollback(SBUF("Evernode: Voting for an already elected candidate."), 1);

                        const uint32_t cur_moment = GET_MOMENT(cur_idx);
                        uint32_t voter_base_count = UINT32_FROM_BUF(&governance_info[VOTER_BASE_COUNT_OFFSET]);
                        const uint64_t last_vote_timestamp = UINT64_FROM_BUF(&candidate_id[CANDIDATE_LAST_VOTE_TIMESTAMP_OFFSET]);
                        const uint32_t last_vote_moment = GET_MOMENT(last_vote_timestamp);
                        uint32_t supported_count = UINT32_FROM_BUF(&candidate_id[CANDIDATE_POSITIVE_VOTE_COUNT_OFFSET]);

                        PREPARE_VOTE_INFO(cur_moment, last_vote_moment, last_vote_timestamp, voter_base_count, governance_info, candidate_id, supported_count, vote_status);

                        if ((candidate_type != PILOTED_MODE_CANDIDATE) ? VOTING_COMPLETED(vote_status) : (vote_status == CANDIDATE_ELECTED))
                        {
                            candidate_id[CANDIDATE_STATUS_OFFSET] = vote_status;
                            op_type = OP_STATUS_CHANGE;
                        }
                        else
                        {
                            const uint32_t candidate_idx = UINT32_FROM_BUF(&candidate_id[CANDIDATE_IDX_OFFSET]);
                            const uint32_t last_vote_candidate_idx = UINT32_FROM_BUF(&governance_info[FOUNDATION_LAST_VOTED_CANDIDATE_IDX]);
                            const uint32_t foundation_vote_moment = GET_MOMENT(UINT64_FROM_BUF(&governance_info[FOUNDATION_LAST_VOTED_TIMESTAMP_OFFSET]));

                            // If this is a new moment last_vote_candidate_idx needed to be reset. So skip this check.
                            if (cur_moment == foundation_vote_moment && candidate_idx <= last_vote_candidate_idx)
                                rollback(SBUF("Evernode: Voting for already voted candidate is not allowed."), 1);
                            // Only one support vote is allowed for new hook candidate per moment.
                            if (candidate_type == NEW_HOOK_CANDIDATE)
                            {
                                if (cur_moment == foundation_vote_moment &&
                                    *(data_ptr + CANDIDATE_VOTE_VALUE_MEMO_OFFSET) == CANDIDATE_SUPPORTED &&
                                    governance_info[FOUNDATION_SUPPORT_VOTE_FLAG_OFFSET] == 1)
                                    rollback(SBUF("Evernode: Only one support vote is allowed per moment."), 1);
                                governance_info[FOUNDATION_SUPPORT_VOTE_FLAG_OFFSET] = *(data_ptr + CANDIDATE_VOTE_VALUE_MEMO_OFFSET) ? 1 : 0;
                            }

                            UINT32_TO_BUF(&candidate_id[CANDIDATE_POSITIVE_VOTE_COUNT_OFFSET], supported_count);

                            // Update the last vote timestamp.
                            UINT64_TO_BUF(&candidate_id[CANDIDATE_LAST_VOTE_TIMESTAMP_OFFSET], cur_ledger_timestamp);

                            candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET] = *(data_ptr + CANDIDATE_VOTE_VALUE_MEMO_OFFSET);

                            UINT32_TO_BUF(&governance_info[FOUNDATION_LAST_VOTED_CANDIDATE_IDX], candidate_idx);
                            UINT64_TO_BUF(&governance_info[FOUNDATION_LAST_VOTED_TIMESTAMP_OFFSET], cur_ledger_timestamp);
                            if (state_foreign_set(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for governance_game info."), 1);
                        }

                        if (state_foreign_set(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not set state for candidate id."), 1);

                        if (op_type == OP_VOTE)
                            accept(SBUF("Evernode: Successfully accepted foundation hook candidate vote."), 0);
                    }
                    else if (op_type == OP_WITHDRAW)
                    {
                        if (!BUFFER_EQUAL_20(candidate_id, account_field))
                            rollback(SBUF("Evernode: Candidate proposal is not owned by this account."), 1);

                        if (BUFFER_EQUAL_32(data_ptr, &governance_info[ELECTED_PROPOSAL_UNIQUE_ID_OFFSET]))
                            rollback(SBUF("Evernode: Trying to withdraw an already elected proposal."), 1);

                        const uint8_t candidate_type = CANDIDATE_TYPE(data_ptr);
                        if (candidate_type == 0)
                            rollback(SBUF("Evernode: Voting for an invalid candidate type."), 1);
                        else if (candidate_type == PILOTED_MODE_CANDIDATE)
                            rollback(SBUF("Evernode: Piloted mode candidate cannot be withdrawn."), 1);

                        const uint32_t life_period = UINT32_FROM_BUF(&governance_configuration[CANDIDATE_LIFE_PERIOD_OFFSET]);
                        const uint64_t created_timestamp = UINT64_FROM_BUF(&candidate_id[CANDIDATE_CREATED_TIMESTAMP_OFFSET]);
                        if (cur_ledger_timestamp - created_timestamp > life_period)
                            rollback(SBUF("Evernode: Trying to withdraw an already expired proposal."), 1);

                        const uint8_t *proposal_fee_ptr = &candidate_id[CANDIDATE_PROPOSAL_FEE_OFFSET];
                        const int64_t proposal_fee = INT64_FROM_BUF(proposal_fee_ptr);

                        // Send the proposal fee to the owner.
                        etxn_reserve(1);
                        PREPARE_PAYMENT_TRUSTLINE_TX(EVR_TOKEN, issuer_accid, proposal_fee, account_field);
                        uint8_t emithash[32];
                        if (emit(SBUF(emithash), SBUF(PAYMENT_TRUSTLINE)) < 0)
                            rollback(SBUF("Evernode: Emitting EVR forward txn failed"), 1);
                        trace(SBUF("emit hash: "), SBUF(emithash), 1);

                        // Clear the proposal states.
                        if (state_foreign_set(0, 0, SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0 || state_foreign_set(0, 0, SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not delete the candidate states."), 1);

                        accept(SBUF("Evernode: Successfully withdrawn Hook candidate proposal."), 0);
                    }
                    else if (op_type == OP_STATUS_CHANGE)
                    {
                        // We accept only the status change transaction from hook heartbeat account.
                        if (!BUFFER_EQUAL_20(heartbeat_accid, account_field))
                            rollback(SBUF("Evernode: Status change is only allowed from heartbeat account."), 1);
                        else if (BUFFER_EQUAL_32((governance_info + ELECTED_PROPOSAL_UNIQUE_ID_OFFSET), data_ptr))
                            rollback(SBUF("Evernode: Status change for an already elected candidate."), 1);

                        vote_status = *(data_ptr + HASH_SIZE);
                    }
                    else if (op_type == OP_HOOK_UPDATE)
                    {
                        if (state_foreign(SBUF(candidate_owner), SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not get candidate owner state."), 1);

                        uint8_t *hook_hash_ptr;
                        // We accept only the hook update transaction from hook heartbeat or registry accounts.
                        if (BUFFER_EQUAL_20(heartbeat_accid, account_field))
                            hook_hash_ptr = &candidate_owner[CANDIDATE_HEARTBEAT_HOOK_HASH_OFFSET];
                        else if (BUFFER_EQUAL_20(registry_accid, account_field))
                            hook_hash_ptr = &candidate_owner[CANDIDATE_REGISTRY_HOOK_HASH_OFFSET];
                        else
                            rollback(SBUF("Evernode: Only heartbeat or registry are allowed to send hook update results."), 1);

                        int is_applied = 0;
                        CHECK_RUNNING_HOOK(account_field, hook_hash_ptr, is_applied);

                        if (!is_applied)
                            rollback(SBUF("Evernode: Elected hook has not yet applied in the child hook."), 1);

                        if (!BUFFER_EQUAL_32(data_ptr, &governance_info[ELECTED_PROPOSAL_UNIQUE_ID_OFFSET]))
                            rollback(SBUF("Evernode: Candidate unique id is invalid."), 1);

                        const uint8_t updated_hook_count = governance_info[UPDATED_HOOK_COUNT_OFFSET];

                        // Update the hook and the grants if one update hook result is already received.
                        if (updated_hook_count == 1)
                        {
                            uint8_t hash_arr[HASH_SIZE * 4];
                            COPY_32BYTES(hash_arr, &candidate_owner[CANDIDATE_GOVERNOR_HOOK_HASH_OFFSET]);
                            CLEAR_32BYTES(&hash_arr[HASH_SIZE]);
                            CLEAR_32BYTES(&hash_arr[HASH_SIZE * 2]);
                            CLEAR_32BYTES(&hash_arr[HASH_SIZE * 3]);

                            etxn_reserve(1);
                            int tx_size;
                            PREPARE_SET_HOOK_WITH_GRANT_TRANSACTION_TX(hash_arr, NAMESPACE, data_ptr,
                                                                       registry_accid,
                                                                       &candidate_owner[CANDIDATE_REGISTRY_HOOK_HASH_OFFSET],
                                                                       heartbeat_accid,
                                                                       &candidate_owner[CANDIDATE_HEARTBEAT_HOOK_HASH_OFFSET],
                                                                       0, 0, 0, 0, 1, tx_size);
                            uint8_t emithash[HASH_SIZE];
                            if (emit(SBUF(emithash), SET_HOOK_TRANSACTION, tx_size) < 0)
                                rollback(SBUF("Evernode: Emitting set hook failed"), 1);
                            trace(SBUF("emit hash: "), SBUF(emithash), 1);

                            // Clear the proposal states.
                            governance_info[UPDATED_HOOK_COUNT_OFFSET] = 0;
                            CLEAR_32BYTES(&governance_info[ELECTED_PROPOSAL_UNIQUE_ID_OFFSET]);
                            UINT64_TO_BUF(&governance_info[PROPOSAL_ELECTED_TIMESTAMP_OFFSET], 0);

                            if (state_foreign_set(0, 0, SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0 || state_foreign_set(0, 0, SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not delete the candidate states."), 1);
                        }
                        else
                            governance_info[UPDATED_HOOK_COUNT_OFFSET] = 1;

                        if (state_foreign_set(governance_info, GOVERNANCE_INFO_VAL_SIZE, SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not set state for governance_game info."), 1);

                        accept(SBUF("Evernode: Accepted the hook update response."), 0);
                    }
                    else if (op_type == OP_GOVERNANCE_MODE_CHANGE)
                    {
                        // We accept only the change mode transaction from foundation account.
                        if (!source_is_foundation)
                            rollback(SBUF("Evernode: Only foundation is allowed to change governance mode."), 1);

                        if (*(data_ptr) <= governance_info[GOVERNANCE_MODE_OFFSET])
                            rollback(SBUF("Evernode: Downgrading the governance mode is not allowed."), 1);

                        if (*(data_ptr) == PILOTED)
                            governance_info[GOVERNANCE_MODE_OFFSET] = PILOTED;
                        else if (*(data_ptr) == CO_PILOTED || *(data_ptr) == AUTO_PILOTED)
                        {
                            governance_info[GOVERNANCE_MODE_OFFSET] = *(data_ptr);
                            // Setup piloted mode hook candidate on auto piloted mode.
                            uint8_t piloted_cand_id[HASH_SIZE] = {0};
                            GET_PILOTED_MODE_CANDIDATE_ID(piloted_cand_id);
                            CANDIDATE_ID_KEY(piloted_cand_id);

                            COPY_20BYTES(candidate_id + CANDIDATE_OWNER_ADDRESS_OFFSET, foundation_accid);
                            UINT32_TO_BUF(candidate_id + CANDIDATE_IDX_OFFSET, 0);
                            COPY_8BYTES(candidate_id + CANDIDATE_SHORT_NAME_OFFSET, PILOTED_MODE_CAND_SHORTNAME);
                            COPY_4BYTES(candidate_id + CANDIDATE_SHORT_NAME_OFFSET + 8, PILOTED_MODE_CAND_SHORTNAME + 8);
                            COPY_BYTE(candidate_id + CANDIDATE_SHORT_NAME_OFFSET + 12, PILOTED_MODE_CAND_SHORTNAME + 12);
                            UINT64_TO_BUF(candidate_id + CANDIDATE_CREATED_TIMESTAMP_OFFSET, cur_ledger_timestamp);
                            CLEAR_20BYTES(candidate_id + CANDIDATE_PROPOSAL_FEE_OFFSET);
                            candidate_id[CANDIDATE_STATUS_OFFSET] = CANDIDATE_REJECTED;
                            UINT64_TO_BUF(&candidate_id[CANDIDATE_STATUS_CHANGE_TIMESTAMP_OFFSET], cur_ledger_timestamp);
                            candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET] = CANDIDATE_REJECTED;

                            if (state_foreign_set(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for piloted mode candidate."), 1);
                        }

                        if (state_foreign_set(governance_info, GOVERNANCE_INFO_VAL_SIZE, SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not set state for governance_game info."), 1);

                        accept(SBUF("Evernode: Successfully accepted governance mode change."), 0);
                    }
                    else if (op_type == OP_DUD_HOST_REPORT)
                    {
                        HOST_ADDR_KEY(data_ptr + DUD_HOST_CANDID_ADDRESS_OFFSET);
                        // Check for registration entry.
                        if (state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) == DOESNT_EXIST)
                            rollback(SBUF("Evernode: This host is not registered."), 1);

                        TOKEN_ID_KEY((uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET)); // Generate token id key.
                        // Check for token id entry.
                        if (state_foreign(SBUF(token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) == DOESNT_EXIST)
                            rollback(SBUF("Evernode: This host is not registered."), 1);

                        uint8_t unique_id[HASH_SIZE] = {0};
                        GET_DUD_HOST_CANDIDATE_ID((data_ptr + DUD_HOST_CANDID_ADDRESS_OFFSET), unique_id);

                        if (!BUFFER_EQUAL_32(data_ptr, unique_id))
                            rollback(SBUF("Evernode: Generated candidate hash is not matched with provided."), 1);

                        CANDIDATE_ID_KEY(unique_id);
                        if (state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) != DOESNT_EXIST)
                            rollback(SBUF("Evernode: This candidate is already proposed"), 1);

                        const uint32_t candidate_idx = (UINT32_FROM_BUF(&governance_info[LAST_CANDIDATE_IDX_OFFSET]) + 1);

                        COPY_20BYTES((candidate_id + CANDIDATE_OWNER_ADDRESS_OFFSET), account_field);
                        UINT32_TO_BUF(&candidate_id[CANDIDATE_IDX_OFFSET], candidate_idx);
                        COPY_20BYTES((candidate_id + CANDIDATE_SHORT_NAME_OFFSET), (data_ptr1 + CANDIDATE_PROPOSE_SHORT_NAME_MEMO_OFFSET));
                        UINT64_TO_BUF(&candidate_id[CANDIDATE_CREATED_TIMESTAMP_OFFSET], cur_ledger_timestamp);
                        INT64_TO_BUF(&candidate_id[CANDIDATE_PROPOSAL_FEE_OFFSET], proposal_fee);
                        UINT32_TO_BUF(&candidate_id[CANDIDATE_POSITIVE_VOTE_COUNT_OFFSET], 0);
                        UINT64_TO_BUF(&candidate_id[CANDIDATE_LAST_VOTE_TIMESTAMP_OFFSET], 0);
                        candidate_id[CANDIDATE_STATUS_OFFSET] = CANDIDATE_REJECTED;
                        UINT64_TO_BUF(&candidate_id[CANDIDATE_STATUS_CHANGE_TIMESTAMP_OFFSET], cur_ledger_timestamp);
                        candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET] = CANDIDATE_REJECTED;

                        // Write to states.
                        if (state_foreign_set(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not set state for candidate."), 1);

                        // Update the new candidate index.
                        UINT32_TO_BUF(&governance_info[LAST_CANDIDATE_IDX_OFFSET], candidate_idx);
                        if (state_foreign_set(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not set governance info state."), 1);

                        accept(SBUF("Evernode: Successfully reported the dud host."), 0);
                    }
                    if (op_type == OP_STATUS_CHANGE)
                    {
                        if (candidate_id[CANDIDATE_STATUS_OFFSET] != vote_status)
                            rollback(SBUF("Evernode: Invalid status sent with the memo."), 1);

                        // For each candidate type we treat differently.
                        if (candidate_type == NEW_HOOK_CANDIDATE || candidate_type == DUD_HOST_CANDIDATE)
                        {
                            const uint8_t epoch_count = reward_configuration[EPOCH_COUNT_OFFSET];
                            const uint32_t first_epoch_reward_quota = UINT32_FROM_BUF(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET]);
                            const uint32_t epoch_reward_amount = UINT32_FROM_BUF(&reward_configuration[EPOCH_REWARD_AMOUNT_OFFSET]);

                            const uint8_t *proposal_fee_ptr = &candidate_id[CANDIDATE_PROPOSAL_FEE_OFFSET];
                            const int64_t proposal_fee = INT64_FROM_BUF(proposal_fee_ptr);

                            // If proposal expired 50% of proposal fee will be rebated to owner.
                            const int64_t reward_amount = (vote_status == CANDIDATE_ELECTED) ? 0 : (vote_status == CANDIDATE_EXPIRED) ? float_multiply(proposal_fee, float_set(-1, 5))
                                                                                                                                      : proposal_fee;
                            const uint64_t cur_moment_start_timestamp = GET_MOMENT_START_INDEX(cur_ledger_timestamp);

                            uint8_t emithash[HASH_SIZE];
                            if (vote_status == CANDIDATE_VETOED || vote_status == CANDIDATE_EXPIRED)
                            {
                                etxn_reserve(reward_amount > 0 ? 2 : 1);

                                if (reward_amount > 0)
                                {
                                    ADD_TO_REWARD_POOL(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, reward_amount);
                                    // Reading the heartbeat-hook account from states (Not yet set to states)
                                    uint8_t heartbeat_hook_accid[ACCOUNT_ID_SIZE];
                                    if (state_foreign(SBUF(heartbeat_hook_accid), SBUF(CONF_HEARTBEAT_ADDR), FOREIGN_REF) < 0)
                                        rollback(SBUF("Evernode: Error getting heartbeat hook address from states."), 1);

                                    PREPARE_HEARTBEAT_FUND_PAYMENT_TX(reward_amount, heartbeat_hook_accid, txid);
                                    if (emit(SBUF(emithash), SBUF(HEARTBEAT_FUND_PAYMENT)) < 0)
                                        rollback(SBUF("Evernode: EVR funding to heartbeat hook account failed."), 1);
                                    trace(SBUF("emit hash: "), SBUF(emithash), 1);
                                }

                                // Clear the proposal states.
                                if (state_foreign_set(0, 0, SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)
                                    rollback(SBUF("Evernode: Could not delete the candidate id state."), 1);

                                // Dud host candidate does not have a owner state.
                                if (candidate_type == NEW_HOOK_CANDIDATE)
                                {
                                    if (state_foreign_set(0, 0, SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) < 0)
                                        rollback(SBUF("Evernode: Could not delete the candidate owner state."), 1);
                                }
                            }
                            else if (vote_status == CANDIDATE_ELECTED)
                            {
                                etxn_reserve(candidate_type == NEW_HOOK_CANDIDATE ? 3 : 2);

                                if (candidate_type == NEW_HOOK_CANDIDATE)
                                {
                                    // Sending hook update trigger transactions to registry and heartbeat hooks.
                                    PREPARE_HOOK_UPDATE_PAYMENT_TX(1, heartbeat_accid, data_ptr);
                                    if (emit(SBUF(emithash), SBUF(HOOK_UPDATE_PAYMENT)) < 0)
                                        rollback(SBUF("Evernode: Emitting heartbeat hook update trigger failed."), 1);

                                    PREPARE_HOOK_UPDATE_PAYMENT_TX(1, registry_accid, data_ptr);
                                    if (emit(SBUF(emithash), SBUF(HOOK_UPDATE_PAYMENT)) < 0)
                                        rollback(SBUF("Evernode: Emitting registry hook update trigger failed."), 1);

                                    // Update the governance info state.
                                    COPY_32BYTES(&governance_info[ELECTED_PROPOSAL_UNIQUE_ID_OFFSET], data_ptr);
                                    UINT64_TO_BUF(&governance_info[PROPOSAL_ELECTED_TIMESTAMP_OFFSET], cur_moment_start_timestamp);
                                    if (state_foreign_set(governance_info, GOVERNANCE_INFO_VAL_SIZE, SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0)
                                        rollback(SBUF("Evernode: Could not set state for governance info."), 1);
                                }
                                else
                                {
                                    PREPARE_DUD_HOST_REMOVE_TX(1, registry_accid, DUD_HOST_REMOVE, (uint8_t *)(STP_CANDIDATE_ID + DUD_HOST_CANDID_ADDRESS_OFFSET), FORMAT_HEX);
                                    if (emit(SBUF(emithash), SBUF(DUD_HOST_REMOVE_TX)) < 0)
                                        rollback(SBUF("Evernode: Emitting registry dud host removal trigger failed."), 1);
                                }
                            }

                            if (vote_status == CANDIDATE_ELECTED || vote_status == CANDIDATE_VETOED || vote_status == CANDIDATE_EXPIRED)
                            {
                                const int64_t rebate_amount = float_sum(proposal_fee, float_negate(reward_amount));
                                uint8_t *tx_ptr;
                                uint32_t tx_size;
                                const uint8_t *memo_data_ptr = ((vote_status == CANDIDATE_VETOED) ? CANDIDATE_VETOED_RES : ((vote_status == CANDIDATE_ELECTED) ? CANDIDATE_ACCEPT_RES : CANDIDATE_EXPIRY_RES));
                                if (float_compare(rebate_amount, float_set(0, 0), COMPARE_GREATER) == 1)
                                {
                                    PREPARE_CANDIDATE_REBATE_PAYMENT_TX(rebate_amount, candidate_id, memo_data_ptr, data_ptr, FORMAT_HEX);
                                    tx_ptr = CANDIDATE_REBATE_PAYMENT;
                                    tx_size = CANDIDATE_REBATE_PAYMENT_TX_SIZE;
                                }
                                else
                                {
                                    PREPARE_CANDIDATE_REBATE_MIN_PAYMENT_TX(1, candidate_id, memo_data_ptr, data_ptr, FORMAT_HEX);
                                    tx_ptr = CANDIDATE_REBATE_MIN_PAYMENT;
                                    tx_size = CANDIDATE_REBATE_MIN_PAYMENT_TX_SIZE;
                                }
                                if (emit(SBUF(emithash), tx_ptr, tx_size) < 0)
                                    rollback(SBUF("Evernode: EVR funding to candidate account failed."), 1);
                                trace(SBUF("emit hash: "), SBUF(emithash), 1);
                            }

                            if (candidate_type == NEW_HOOK_CANDIDATE)
                                accept(SBUF("Evernode: New hook candidate status changed."), vote_status);
                            else
                                accept(SBUF("Evernode: Dud host candidate status changed."), vote_status);
                        }
                        else if (candidate_type == PILOTED_MODE_CANDIDATE && vote_status == CANDIDATE_ELECTED)
                        {
                            // Change governance the mode.
                            governance_info[GOVERNANCE_MODE_OFFSET] = PILOTED;
                            if (state_foreign_set(governance_info, GOVERNANCE_INFO_VAL_SIZE, SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for governance_game info."), 1);

                            // Clear piloted mode candidate if piloted mode is elected.
                            if (state_foreign_set(0, 0, SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for piloted mode candidate."), 1);

                            accept(SBUF("Evernode: Piloted mode candidate status changed."), vote_status);
                        }
                    }
                }
            }
        }
    }

    accept(SBUF("Evernode: Transaction is not handled."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}