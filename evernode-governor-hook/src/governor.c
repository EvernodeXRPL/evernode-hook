#include "governor.h"

/**
 * Only the incoming transactions are handled. Outgoing transactions are accepted directly.
 * Manages governance related operations for Evernode.
 * Handles hook state initialization, Hook candidate / Dud host candidate / Piloted mode candidate management and
 * Applying new hooks and triggering other hooks to update themselves.
 * Contains all the states. Foreign state access is granted to other accounts which contains registry and heartbeat hooks.
 * Supported transaction types: Payment(IOU|XRP)
 * Transaction Params:
 * Key: 4556520100000000000000000000000000000000000000000000000000000002
 * Value: <Predefined evernode operation identifier>
 * Key: 4556520100000000000000000000000000000000000000000000000000000003
 * Value: <Required data to invoke the respective operation {sliced[0:128]}>
 * Key: 4556520100000000000000000000000000000000000000000000000000000004
 * Value: <Required data to invoke the respective operation {sliced[128:]}>
 * If param 4556520100000000000000000000000000000000000000000000000000000002 does not contain a predefined identifier,
 * transaction is accepted directly.
 * If it contains a predefined identifier but the rest of the params contains invalid data or does not contain required data to invoke the operation,
 * transaction is rejected.
 */
int64_t hook(uint32_t reserved)
{
    // Getting the hook account id.
    unsigned char hook_accid[20];
    hook_account((uint32_t)hook_accid, 20);

    // Next fetch the sfAccount field from the originating transaction
    uint8_t account_field[ACCOUNT_ID_SIZE];
    int32_t account_field_len = otxn_field(SBUF(account_field), sfAccount);

    // ASSERT_FAILURE_MSG >> sfAccount field is missing.
    ASSERT(account_field_len >= 20);

    /**
     * Accept
     * - any transaction that is not a payment.
     * - any outgoing transactions without further processing.
     */
    int64_t txn_type = otxn_type();
    if (txn_type != ttPAYMENT || BUFFER_EQUAL_20(hook_accid, account_field))
    {
        // PERMIT_MSG >> Transaction is not handled.
        PERMIT();
    }

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

    // ASSERT_FAILURE_MSG >> Could not get moment base info state.
    ASSERT(!(moment_base_info_state_res < 0 && moment_base_info_state_res != DOESNT_EXIST));

    uint64_t moment_base_idx = UINT64_FROM_BUF_LE(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);
    uint32_t prev_transition_moment = UINT32_FROM_BUF_LE(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]);
    // If state does not exist, take the moment type from default constant.
    uint8_t cur_moment_type = (moment_base_info_state_res == DOESNT_EXIST) ? DEF_MOMENT_TYPE : moment_base_info[MOMENT_TYPE_OFFSET];
    uint64_t cur_idx = cur_moment_type == TIMESTAMP_MOMENT_TYPE ? cur_ledger_timestamp : cur_ledger_seq;

    // Take the moment size from config.
    uint16_t moment_size;
    int64_t moment_size_res = state_foreign(&moment_size, sizeof(moment_size), SBUF(CONF_MOMENT_SIZE), FOREIGN_REF);

    // ASSERT_FAILURE_MSG >> Could not get moment size.
    ASSERT(!(moment_size_res < 0 && moment_size_res != DOESNT_EXIST));

    // <fee_base_avg(uint32_t)><avg_changed_idx(uint64_t)><avg_accumulator(uint32_t)><counter(uint16_t)>
    uint8_t trx_fee_base_info[TRX_FEE_BASE_INFO_VAL_SIZE] = {0};
    const int fee_base_info_state_res = state_foreign(SBUF(trx_fee_base_info), SBUF(STK_TRX_FEE_BASE_INFO), FOREIGN_REF);

    // ASSERT_FAILURE_MSG >> Error getting transaction fee base info state.
    ASSERT(!(fee_base_info_state_res < 0 && fee_base_info_state_res != DOESNT_EXIST));

    const int64_t cur_fee_base = fee_base();
    const uint32_t fee_avg = (fee_base_info_state_res >= 0) ? UINT32_FROM_BUF_LE(&trx_fee_base_info[FEE_BASE_AVG_OFFSET]) : (uint32_t)cur_fee_base;

    // <busyness_detect_period(uint32_t)><busyness_detect_average(uint16_t)>
    uint8_t network_configuration[NETWORK_CONFIGURATION_VAL_SIZE] = {0};
    const int net_config_state_res = state_foreign(SBUF(network_configuration), SBUF(CONF_NETWORK_CONFIGURATION), FOREIGN_REF);

    // ASSERT_FAILURE_MSG >> Error getting network configuration state.
    ASSERT(!(net_config_state_res < 0 && net_config_state_res != DOESNT_EXIST));

    uint16_t network_busyness_detect_average = (net_config_state_res >= 0) ? UINT16_FROM_BUF_LE(&network_configuration[NETWORK_BUSYNESS_DETECT_AVERAGE_OFFSET]) : 0;

    // Get transaction hash(id).
    uint8_t txid[HASH_SIZE];
    const int32_t txid_len = otxn_id(SBUF(txid), 0);

    // ASSERT_FAILURE_MSG >> transaction id missing.
    ASSERT(txid_len == HASH_SIZE);

    uint8_t event_type[MAX_EVENT_TYPE_SIZE];
    const int64_t event_type_len = otxn_param(SBUF(event_type), SBUF(PARAM_EVENT_TYPE_KEY));
    if (event_type_len == DOESNT_EXIST)
    {
        // PERMIT_MSG >> Transaction is not handled.
        PERMIT();
    }
    // ASSERT_FAILURE_MSG >> Error getting the event type param.
    ASSERT(event_type_len >= 0);

    // specifically we're interested in the amount sent
    otxn_slot(1);
    int64_t amt_slot = slot_subfield(1, sfAmount, 0);

    uint8_t origin_op_type = OP_NONE;
    uint8_t op_type = OP_NONE;
    uint8_t redirect_op_type = OP_NONE;

    uint8_t source_is_foundation = 0;
    const uint64_t zero = 0;

    // ASSERT_FAILURE_MSG >> Could not slot otxn.sfAmount
    ASSERT(amt_slot >= 0);

    int64_t is_xrp = slot_type(amt_slot, 1);

    // ASSERT_FAILURE_MSG >> Could not determine sent amount type.
    ASSERT(is_xrp >= 0);

    if (is_xrp)
    {
        // Host initialization.
        if (EQUAL_INITIALIZE(event_type, event_type_len))
            op_type = OP_INITIALIZE;
        // Change Governance Mode.
        else if (EQUAL_CHANGE_GOVERNANCE_MODE(event_type, event_type_len))
            op_type = OP_GOVERNANCE_MODE_CHANGE;
        // Invoke Governor on host heartbeat.
        else if (EQUAL_CANDIDATE_STATUS_CHANGE(event_type, event_type_len))
            op_type = OP_STATUS_CHANGE;
        // Invoke Governor on host removal.
        else if (EQUAL_ORPHAN_CANDIDATE_REMOVE(event_type, event_type_len))
            op_type = OP_REMOVE_ORPHAN_CANDIDATE;
        // Hook candidate withdraw request.
        else if (EQUAL_CANDIDATE_WITHDRAW(event_type, event_type_len))
            op_type = OP_WITHDRAW;
        // Child hook update results.
        else if (EQUAL_HOOK_UPDATE_RES(event_type, event_type_len))
            op_type = OP_HOOK_UPDATE;
        // Invoke Governor on a removal of host who is linked to a dud host candidate.
        else if (EQUAL_LINKED_CANDIDATE_REMOVE(event_type, event_type_len))
            op_type = OP_REMOVE_LINKED_CANDIDATE;
    }
    else // IOU payments.
    {
        // Proposing a Hook Candidate.
        if (EQUAL_CANDIDATE_PROPOSE(event_type, event_type_len))
            op_type = OP_PROPOSE;
        // Dud host report.
        else if (EQUAL_DUD_HOST_REPORT(event_type, event_type_len))
            op_type = OP_DUD_HOST_REPORT;
    }

    if (op_type != OP_NONE)
    {
        // All the events should contain an event data.
        uint8_t event_data[MAX_EVENT_DATA_SIZE];
        int64_t event_data_len = otxn_param(SBUF(event_data), SBUF(PARAM_EVENT_DATA1_KEY));

        // ASSERT_FAILURE_MSG >> Error getting the event data param.
        ASSERT(event_data_len >= 0);

        if (op_type == OP_REMOVE_ORPHAN_CANDIDATE)
        {
            origin_op_type = OP_REMOVE_ORPHAN_CANDIDATE;
            op_type = OP_STATUS_CHANGE;
        }

        // <token_id(32)><country_code(2)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)><no_of_total_instances(4)><no_of_active_instances(4)>
        // <last_heartbeat_index(8)><version(3)><registration_timestamp(8)><transfer_flag(1)><last_vote_candidate_idx(4)><support_vote_sent(1)>
        uint8_t host_addr[HOST_ADDR_VAL_SIZE];
        // <host_address(20)><cpu_model_name(40)><cpu_count(2)><cpu_speed(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><accumulated_reward_amount(8)>
        uint8_t token_id[TOKEN_ID_VAL_SIZE];
        // <eligibility_period(uint32_t)><candidate_life_period<uint32_t)><candidate_election_period<uint32_t)><candidate_support_average<uint16_t)>
        uint8_t governance_configuration[GOVERNANCE_CONFIGURATION_VAL_SIZE] = {0};
        uint8_t issuer_accid[ACCOUNT_ID_SIZE] = {0};
        uint8_t foundation_accid[ACCOUNT_ID_SIZE] = {0};
        if (op_type == OP_PROPOSE || op_type == OP_STATUS_CHANGE || op_type == OP_WITHDRAW || op_type == OP_DUD_HOST_REPORT || op_type == OP_GOVERNANCE_MODE_CHANGE || op_type == OP_REMOVE_LINKED_CANDIDATE)
        {
            // ASSERT_FAILURE_MSG >> Could not get governance configuration state.
            ASSERT(state_foreign(SBUF(governance_configuration), SBUF(CONF_GOVERNANCE_CONFIGURATION), FOREIGN_REF) >= 0);

            // ASSERT_FAILURE_MSG >> Could not get issuer account id.
            ASSERT(state_foreign(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR), FOREIGN_REF) >= 0);

            // ASSERT_FAILURE_MSG >> Could not get state for foundation account.
            ASSERT(state_foreign(foundation_accid, ACCOUNT_ID_SIZE, SBUF(CONF_FOUNDATION_ADDR), FOREIGN_REF) >= 0);

            source_is_foundation = BUFFER_EQUAL_20(foundation_accid, account_field);
        }

        // Validation check for participants other than the foundation address
        if ((op_type == OP_PROPOSE || op_type == OP_DUD_HOST_REPORT) && !source_is_foundation)
        {
            const uint32_t min_eligibility_period = UINT32_FROM_BUF_LE(&governance_configuration[ELIGIBILITY_PERIOD_OFFSET]);

            HOST_ADDR_KEY(account_field);
            // Check for registration entry.
            // ASSERT_FAILURE_MSG >> This host is not registered.
            ASSERT(state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) != DOESNT_EXIST);

            uint8_t eligible_for_governance = 0;
            uint8_t do_rollback = 1;
            VALIDATE_GOVERNANCE_ELIGIBILITY(host_addr, cur_ledger_timestamp, min_eligibility_period, eligible_for_governance, do_rollback);
        }

        // <governance_mode(1)><last_candidate_idx(4)><voter_base_count(4)><voter_base_count_changed_timestamp(8)><foundation_last_voted_candidate_idx(4)><elected_proposal_unique_id(32)>
        // <proposal_elected_timestamp(8)><updated_hook_count(1)><foundation_vote_flag(1)>
        uint8_t governance_info[GOVERNANCE_INFO_VAL_SIZE];
        if (op_type == OP_STATUS_CHANGE || op_type == OP_HOOK_UPDATE || op_type == OP_GOVERNANCE_MODE_CHANGE || op_type == OP_PROPOSE || op_type == OP_DUD_HOST_REPORT)
        {
            // ASSERT_FAILURE_MSG >> Could not get state governance_game info.
            ASSERT(state_foreign(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) != DOESNT_EXIST);
        }

        uint8_t heartbeat_accid[ACCOUNT_ID_SIZE] = {0};
        uint8_t registry_accid[ACCOUNT_ID_SIZE] = {0};
        // <owner_address(20)><candidate_idx(4)><short_name(20)><created_timestamp(8)><proposal_fee(8)><positive_vote_count(4)>
        // <last_vote_timestamp(8)><status(1)><status_change_timestamp(8)><foundation_vote_status(1)>
        uint8_t candidate_id[CANDIDATE_ID_VAL_SIZE];
        // <GOVERNOR_HASH(32)><REGISTRY_HASH(32)><HEARTBEAT_HASH(32)>
        uint8_t candidate_owner[CANDIDATE_OWNER_VAL_SIZE];
        if (op_type == OP_STATUS_CHANGE || op_type == OP_HOOK_UPDATE || op_type == OP_WITHDRAW || op_type == OP_REMOVE_LINKED_CANDIDATE)
        {
            // ASSERT_FAILURE_MSG >> Could not get heartbeat or registry account id.
            ASSERT(!(state_foreign(SBUF(heartbeat_accid), SBUF(CONF_HEARTBEAT_ADDR), FOREIGN_REF) < 0 || state_foreign(SBUF(registry_accid), SBUF(CONF_REGISTRY_ADDR), FOREIGN_REF) < 0));

            CANDIDATE_ID_KEY(event_data);

            // ASSERT_FAILURE_MSG >> Error getting a candidate for the given id.
            ASSERT(state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) >= 0);
            // As first 20 bytes of "candidate_id" represents owner address.
            CANDIDATE_OWNER_KEY(candidate_id);
        }

        // <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)>
        uint8_t reward_info[REWARD_INFO_VAL_SIZE];
        // <epoch_count(uint8_t)><first_epoch_reward_quota(uint32_t)><epoch_reward_amount(uint32_t)><reward_start_moment(uint32_t)><accumulated_reward_frequency(uint16_t)>
        uint8_t reward_configuration[REWARD_CONFIGURATION_VAL_SIZE];
        if (op_type == OP_PROPOSE || op_type == OP_DUD_HOST_REPORT || op_type == OP_STATUS_CHANGE || op_type == OP_HOOK_UPDATE)
        {
            // ASSERT_FAILURE_MSG >> Could not get reward configuration or reward info states.
            ASSERT(!(state_foreign(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO), FOREIGN_REF) < 0 ||
                     state_foreign(reward_configuration, REWARD_CONFIGURATION_VAL_SIZE, SBUF(CONF_REWARD_CONFIGURATION), FOREIGN_REF) < 0));
        }

        int64_t proposal_fee;
        if (op_type == OP_PROPOSE || op_type == OP_DUD_HOST_REPORT)
        {
            // BEGIN : Proposal Fee validations.
            uint8_t amount_buffer[AMOUNT_BUF_SIZE];
            const int64_t result = slot(SBUF(amount_buffer), amt_slot);

            // ASSERT_FAILURE_MSG >> Could not dump sfAmount
            ASSERT(result == AMOUNT_BUF_SIZE);
            const int64_t float_amt = slot_float(amt_slot);

            // ASSERT_FAILURE_MSG >> Could not parse amount.
            ASSERT(float_amt >= 0);

            // Currency should be EVR.
            // ASSERT_FAILURE_MSG >> Currency should be EVR for candidate proposal.
            ASSERT(IS_EVR(amount_buffer, issuer_accid));

            const uint8_t epoch_count = reward_configuration[EPOCH_COUNT_OFFSET];
            const uint32_t first_epoch_reward_quota = UINT32_FROM_BUF_LE(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET]);

            const uint8_t epoch = reward_info[EPOCH_OFFSET];
            uint32_t reward_quota;
            GET_EPOCH_REWARD_QUOTA(epoch, first_epoch_reward_quota, reward_quota);

            // Proposal fee for dud host remove would be 25% of reward quota.
            proposal_fee = float_set(0, reward_quota);
            proposal_fee = op_type == OP_PROPOSE ? proposal_fee : float_multiply(proposal_fee, float_set(-2, 25));

            // ASSERT_FAILURE_MSG >> Proposal fee amount is less than the minimum fee.
            ASSERT(float_compare(float_amt, proposal_fee, COMPARE_LESS) != 1);

            proposal_fee = float_amt;
            // END : Proposal Fee validations.
        }

        if (op_type == OP_INITIALIZE)
        {
            uint8_t initializer_accid[ACCOUNT_ID_SIZE];
            const int initializer_accid_len = util_accid(SBUF(initializer_accid), HOOK_INITIALIZER_ADDR, 35);
            // ASSERT_FAILURE_MSG >> Could not convert initializer account id.
            ASSERT(initializer_accid_len >= ACCOUNT_ID_SIZE);

            // We accept only the init transaction from hook initializer account

            // ASSERT_FAILURE_MSG >> Only initializer is allowed to initialize states.
            ASSERT(BUFFER_EQUAL_20(initializer_accid, account_field));

            // ASSERT_FAILURE_MSG >> Memo data should contain foundation cold wallet, registry heartbeat hook and issuer addresses.
            ASSERT(event_data_len == (4 * ACCOUNT_ID_SIZE));

            uint8_t *issuer_ptr = event_data;
            uint8_t *foundation_ptr = event_data + ACCOUNT_ID_SIZE;
            uint8_t *registry_hook_ptr = event_data + (2 * ACCOUNT_ID_SIZE);
            uint8_t *heartbeat_hook_ptr = event_data + (3 * ACCOUNT_ID_SIZE);

            int already_initialized = 0; // For Beta test purposes

            // First check if the states are already initialized by checking lastly added state key for existence.
            if (state_foreign(governance_info, GOVERNANCE_INFO_VAL_SIZE, SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) != DOESNT_EXIST)
            {
                already_initialized = 1;

                const uint64_t zero = 0;
                // Initialize the state.
            }

            //// Configuration states. ////

            // ASSERT_FAILURE_MSG >> Could not set state for issuer account.
            ASSERT(state_foreign_set(issuer_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_ISSUER_ADDR), FOREIGN_REF) >= 0);

            // ASSERT_FAILURE_MSG >> Could not set state for foundation account.
            ASSERT(state_foreign_set(foundation_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_FOUNDATION_ADDR), FOREIGN_REF) >= 0);

            // ASSERT_FAILURE_MSG >> Could not set state for heartbeat hook account.
            ASSERT(state_foreign_set(heartbeat_hook_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_HEARTBEAT_ADDR), FOREIGN_REF) >= 0);

            // ASSERT_FAILURE_MSG >> Could not set state for registry hook account.
            ASSERT(state_foreign_set(registry_hook_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_REGISTRY_ADDR), FOREIGN_REF) >= 0);

            SET_UINT_STATE_VALUE(DEF_MOMENT_SIZE, CONF_MOMENT_SIZE, "Evernode: Could not initialize state for moment size.");
            moment_size = DEF_MOMENT_SIZE;

            reward_configuration[EPOCH_COUNT_OFFSET] = DEF_EPOCH_COUNT;
            UINT32_TO_BUF_LE(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET], DEF_FIRST_EPOCH_REWARD_QUOTA);
            UINT32_TO_BUF_LE(&reward_configuration[EPOCH_REWARD_AMOUNT_OFFSET], DEF_EPOCH_REWARD_AMOUNT);
            UINT32_TO_BUF_LE(&reward_configuration[REWARD_START_MOMENT_OFFSET], DEF_REWARD_START_MOMENT);

            if (already_initialized == 0)
            {
                // Singleton states. ////

                SET_UINT_STATE_VALUE(zero, STK_HOST_COUNT, "Evernode: Could not initialize state for host count.");
                SET_UINT_STATE_VALUE(DEF_HOST_REG_FEE, STK_HOST_REG_FEE, "Evernode: Could not initialize state for reg fee.");
                SET_UINT_STATE_VALUE(DEF_MAX_REG, STK_MAX_REG, "Evernode: Could not initialize state for maximum registrants.");

                moment_base_info[MOMENT_TYPE_OFFSET] = DEF_MOMENT_TYPE;
                UINT64_TO_BUF_LE(&moment_base_info[MOMENT_BASE_POINT_OFFSET], cur_idx);
                moment_base_idx = cur_idx;

                // ASSERT_FAILURE_MSG >> Could not initialize state for moment base info.
                ASSERT(state_foreign_set(SBUF(moment_base_info), SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF) >= 0);

                const uint32_t cur_moment = GET_MOMENT(cur_idx);
                reward_info[EPOCH_OFFSET] = 1;
                UINT32_TO_BUF_LE(&reward_info[SAVED_MOMENT_OFFSET], cur_moment);
                UINT32_TO_BUF_LE(&reward_info[PREV_MOMENT_ACTIVE_HOST_COUNT_OFFSET], zero);
                UINT32_TO_BUF_LE(&reward_info[CUR_MOMENT_ACTIVE_HOST_COUNT_OFFSET], zero);
                const int64_t epoch_pool = float_set(0, DEF_EPOCH_REWARD_AMOUNT);
                INT64_TO_BUF_LE(&reward_info[EPOCH_POOL_OFFSET], epoch_pool);

                // ASSERT_FAILURE_MSG >> Could not set state for reward info.
                ASSERT(state_foreign_set(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO), FOREIGN_REF) >= 0);

                governance_info[EPOCH_OFFSET] = PILOTED;

                // ASSERT_FAILURE_MSG >> Could not set state for governance info.
                ASSERT(state_foreign_set(governance_info, GOVERNANCE_INFO_VAL_SIZE, SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) >= 0);
            }

            if (already_initialized == 1)
            {
                // goverannce flow
                UINT32_TO_BUF_LE(&governance_configuration[ELIGIBILITY_PERIOD_OFFSET], DEF_GOVERNANCE_ELIGIBILITY_PERIOD);
                UINT32_TO_BUF_LE(&governance_configuration[CANDIDATE_LIFE_PERIOD_OFFSET], DEF_CANDIDATE_LIFE_PERIOD);
                UINT32_TO_BUF_LE(&governance_configuration[CANDIDATE_ELECTION_PERIOD_OFFSET], DEF_CANDIDATE_ELECTION_PERIOD);
                UINT16_TO_BUF_LE(&governance_configuration[CANDIDATE_SUPPORT_AVERAGE_OFFSET], DEF_CANDIDATE_SUPPORT_AVERAGE);

                // ASSERT_FAILURE_MSG >> Could not set state for governance configuration.
                ASSERT(state_foreign_set(SBUF(governance_configuration), SBUF(CONF_GOVERNANCE_CONFIGURATION), FOREIGN_REF) >= 0);
            }

            redirect_op_type = OP_CHANGE_CONFIGURATION;
        }
        else if (op_type == OP_PROPOSE)
        {
            // Continue loading data into the buffer from other params.
            const int64_t event_data2_len = otxn_param(event_data + event_data_len, MAX_HOOK_PARAM_SIZE, SBUF(PARAM_EVENT_DATA2_KEY));

            // ASSERT_FAILURE_MSG >> Error getting the event data 2 param.
            ASSERT(event_data2_len >= 0);
            event_data_len += event_data2_len;

            int hooks_exists = 0;
            IS_HOOKS_VALID((event_data + CANDIDATE_PROPOSE_KEYLETS_PARAM_OFFSET), hooks_exists);

            // ASSERT_FAILURE_MSG >> Provided hooks are not valid.
            ASSERT(hooks_exists != 0);

            CANDIDATE_OWNER_KEY(account_field);

            // ASSERT_FAILURE_MSG >> This owner has already created a proposal.
            ASSERT(state_foreign(SBUF(candidate_owner), SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) == DOESNT_EXIST);

            uint8_t unique_id[HASH_SIZE] = {0};
            GET_NEW_HOOK_CANDIDATE_ID(event_data + CANDIDATE_PROPOSE_HASHES_PARAM_OFFSET, CANDIDATE_PROPOSE_KEYLETS_PARAM_OFFSET, unique_id);

            // ASSERT_FAILURE_MSG >> Generated candidate hash is not matched with provided.
            ASSERT(BUFFER_EQUAL_32((event_data + CANDIDATE_PROPOSE_UNIQUE_ID_PARAM_OFFSET), unique_id));

            CANDIDATE_ID_KEY(unique_id);

            // ASSERT_FAILURE_MSG >> This candidate is already proposed
            ASSERT(state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) == DOESNT_EXIST);

            const uint32_t candidate_idx = (UINT32_FROM_BUF_LE(&governance_info[LAST_CANDIDATE_IDX_OFFSET]) + 1);

            COPY_20BYTES((candidate_id + CANDIDATE_OWNER_ADDRESS_OFFSET), account_field);
            UINT32_TO_BUF_LE(&candidate_id[CANDIDATE_IDX_OFFSET], candidate_idx);
            COPY_20BYTES((candidate_id + CANDIDATE_SHORT_NAME_OFFSET), (event_data + CANDIDATE_PROPOSE_SHORT_NAME_PARAM_OFFSET));
            UINT64_TO_BUF_LE(&candidate_id[CANDIDATE_CREATED_TIMESTAMP_OFFSET], cur_ledger_timestamp);
            INT64_TO_BUF_LE(&candidate_id[CANDIDATE_PROPOSAL_FEE_OFFSET], proposal_fee);
            UINT32_TO_BUF_LE(&candidate_id[CANDIDATE_POSITIVE_VOTE_COUNT_OFFSET], zero);
            UINT64_TO_BUF_LE(&candidate_id[CANDIDATE_LAST_VOTE_TIMESTAMP_OFFSET], zero);
            candidate_id[CANDIDATE_STATUS_OFFSET] = CANDIDATE_REJECTED;
            UINT64_TO_BUF_LE(&candidate_id[CANDIDATE_STATUS_CHANGE_TIMESTAMP_OFFSET], cur_ledger_timestamp);
            candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET] = CANDIDATE_REJECTED;

            // Write to states.
            COPY_CANDIDATE_HASHES(candidate_owner, event_data);

            // ASSERT_FAILURE_MSG >> Could not set state for candidate.
            ASSERT(!(state_foreign_set(SBUF(candidate_owner), SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) < 0 || state_foreign_set(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0));

            // Update the new candidate index.
            UINT32_TO_BUF_LE(&governance_info[LAST_CANDIDATE_IDX_OFFSET], candidate_idx);

            // ASSERT_FAILURE_MSG >> Could not set governance info state.
            ASSERT(state_foreign_set(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) >= 0);

            // PERMIT_MSG >> Successfully accepted Hook candidate proposal.
            PERMIT();
        }
        else if (op_type == OP_WITHDRAW)
        {
            // ASSERT_FAILURE_MSG >> Candidate proposal is not owned by this account.
            ASSERT(BUFFER_EQUAL_20(candidate_id, account_field));

            // ASSERT_FAILURE_MSG >> Trying to withdraw an already closed proposal.
            ASSERT(!VOTING_COMPLETED(candidate_id[CANDIDATE_STATUS_OFFSET]));

            const uint8_t candidate_type = CANDIDATE_TYPE(event_data);

            // ASSERT_FAILURE_MSG >> Piloted mode candidate cannot be withdrawn.
            ASSERT(candidate_type != PILOTED_MODE_CANDIDATE);

            // ASSERT_FAILURE_MSG >> Withdrawing an an invalid candidate type.
            ASSERT(candidate_type == DUD_HOST_CANDIDATE || candidate_type == NEW_HOOK_CANDIDATE);

            const uint32_t life_period = UINT32_FROM_BUF_LE(&governance_configuration[CANDIDATE_LIFE_PERIOD_OFFSET]);
            const uint64_t created_timestamp = UINT64_FROM_BUF_LE(&candidate_id[CANDIDATE_CREATED_TIMESTAMP_OFFSET]);

            // ASSERT_FAILURE_MSG >> Trying to withdraw an already expired proposal.
            ASSERT(cur_ledger_timestamp - created_timestamp <= life_period);

            redirect_op_type = OP_REMOVE;
        }

        else if (op_type == OP_REMOVE_LINKED_CANDIDATE)
        {
            // ASSERT_FAILURE_MSG >> Candidate removal request trx has not being initiated via registry.
            ASSERT(BUFFER_EQUAL_20(registry_accid, account_field));

            const uint8_t candidate_type = CANDIDATE_TYPE(event_data);
            // ASSERT_FAILURE_MSG >> Provided candidate is not a dud host candidate.
            ASSERT(candidate_type == DUD_HOST_CANDIDATE);

            // Check whether registration entry is removed or not.
            HOST_ADDR_KEY(event_data + DUD_HOST_CANDID_ADDRESS_OFFSET);

            // ASSERT_FAILURE_MSG >> This host's state entry is not removed or it has not been transferred correctly.
            ASSERT((state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) <= 0 || host_addr[HOST_TRANSFER_FLAG_OFFSET] == TRANSFER_FLAG));

            redirect_op_type = OP_REMOVE;
        }
        else if (op_type == OP_HOOK_UPDATE)
        {
            // ASSERT_FAILURE_MSG >> Could not get candidate owner state.
            ASSERT(state_foreign(SBUF(candidate_owner), SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) >= 0);

            // ASSERT_FAILURE_MSG >> Only heartbeat or registry are allowed to send hook update results.
            ASSERT(BUFFER_EQUAL_20(heartbeat_accid, account_field) || BUFFER_EQUAL_20(registry_accid, account_field));

            uint8_t *hook_hash_ptr;
            // We accept only the hook update transaction from hook heartbeat or registry accounts.
            if (BUFFER_EQUAL_20(heartbeat_accid, account_field))
                hook_hash_ptr = &candidate_owner[CANDIDATE_HEARTBEAT_HOOK_HASH_OFFSET];
            else
                hook_hash_ptr = &candidate_owner[CANDIDATE_REGISTRY_HOOK_HASH_OFFSET];

            int is_applied = 0;
            CHECK_RUNNING_HOOK(account_field, hook_hash_ptr, is_applied);

            // ASSERT_FAILURE_MSG >> Elected hook has not yet applied in the child hook.
            ASSERT(is_applied);

            // ASSERT_FAILURE_MSG >> Candidate unique id is invalid.
            ASSERT(BUFFER_EQUAL_32(event_data, &governance_info[ELECTED_PROPOSAL_UNIQUE_ID_OFFSET]));

            // ASSERT_FAILURE_MSG >> Trying to apply a candidate which is not elected.
            ASSERT(candidate_id[CANDIDATE_STATUS_OFFSET] == CANDIDATE_ELECTED);

            const uint8_t updated_hook_count = governance_info[UPDATED_HOOK_COUNT_OFFSET];

            // Update the hook and the grants if one update hook result is already received.
            if (updated_hook_count == 1)
            {
                uint8_t hash_arr[HASH_SIZE * 4];
                COPY_32BYTES(hash_arr, &candidate_owner[CANDIDATE_GOVERNOR_HOOK_HASH_OFFSET]);

                etxn_reserve(1);
                int tx_size;
                PREPARE_SET_HOOK_WITH_GRANT_TRANSACTION_TX(hash_arr, NAMESPACE, event_data,
                                                           registry_accid,
                                                           &candidate_owner[CANDIDATE_REGISTRY_HOOK_HASH_OFFSET],
                                                           heartbeat_accid,
                                                           &candidate_owner[CANDIDATE_HEARTBEAT_HOOK_HASH_OFFSET],
                                                           0, 0, 0, 0, 1, tx_size);
                uint8_t emithash[HASH_SIZE];
                // ASSERT_FAILURE_MSG >> Emitting set hook failed
                ASSERT(emit(SBUF(emithash), SET_HOOK_TRANSACTION, tx_size) >= 0);
                trace(SBUF("emit hash: "), SBUF(emithash), 1);

                // Clear the proposal states.
                governance_info[UPDATED_HOOK_COUNT_OFFSET] = 0;

                // ASSERT_FAILURE_MSG >> Could not delete the candidate states.
                ASSERT(!(state_foreign_set(0, 0, SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0 || state_foreign_set(0, 0, SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) < 0));
            }
            else
                governance_info[UPDATED_HOOK_COUNT_OFFSET] = 1;

            // ASSERT_FAILURE_MSG >> Could not set state for governance_game info.
            ASSERT(state_foreign_set(governance_info, GOVERNANCE_INFO_VAL_SIZE, SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) >= 0);

            if (updated_hook_count == 1)
                redirect_op_type = OP_CHANGE_CONFIGURATION;
            else
            {
                // PERMIT_MSG >> Accepted the hook update response.
                PERMIT();
            }
        }
        else if (op_type == OP_GOVERNANCE_MODE_CHANGE)
        {

            // We accept only the change mode transaction from foundation account.
            // ASSERT_FAILURE_MSG >> Only foundation is allowed to change governance mode.
            ASSERT(source_is_foundation);

            // ASSERT_FAILURE_MSG >> Downgrading the governance mode is not allowed.
            ASSERT(*(event_data) > governance_info[GOVERNANCE_MODE_OFFSET]);

            if (*(event_data) == PILOTED)
                governance_info[GOVERNANCE_MODE_OFFSET] = PILOTED;
            else if (*(event_data) == CO_PILOTED || *(event_data) == AUTO_PILOTED)
            {
                governance_info[GOVERNANCE_MODE_OFFSET] = *(event_data);
                // Setup piloted mode hook candidate on auto piloted mode.
                uint8_t piloted_cand_id[HASH_SIZE] = {0};
                GET_PILOTED_MODE_CANDIDATE_ID(piloted_cand_id);
                CANDIDATE_ID_KEY(piloted_cand_id);

                COPY_20BYTES(candidate_id + CANDIDATE_OWNER_ADDRESS_OFFSET, foundation_accid);
                UINT32_TO_BUF_LE(&candidate_id[CANDIDATE_IDX_OFFSET], zero);
                COPY_8BYTES(candidate_id + CANDIDATE_SHORT_NAME_OFFSET, PILOTED_MODE_CAND_SHORTNAME);
                COPY_4BYTES(candidate_id + CANDIDATE_SHORT_NAME_OFFSET + 8, PILOTED_MODE_CAND_SHORTNAME + 8);
                COPY_BYTE(candidate_id + CANDIDATE_SHORT_NAME_OFFSET + 12, PILOTED_MODE_CAND_SHORTNAME + 12);
                UINT64_TO_BUF_LE(&candidate_id[CANDIDATE_CREATED_TIMESTAMP_OFFSET], cur_ledger_timestamp);
                candidate_id[CANDIDATE_STATUS_OFFSET] = CANDIDATE_REJECTED;
                UINT64_TO_BUF_LE(&candidate_id[CANDIDATE_STATUS_CHANGE_TIMESTAMP_OFFSET], cur_ledger_timestamp);
                candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET] = CANDIDATE_REJECTED;

                // ASSERT_FAILURE_MSG >> Could not set state for piloted mode candidate.
                ASSERT(state_foreign_set(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) >= 0);
            }

            // ASSERT_FAILURE_MSG >> Could not set state for governance_game info.
            ASSERT(state_foreign_set(governance_info, GOVERNANCE_INFO_VAL_SIZE, SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) >= 0);

            // PERMIT_MSG >> Successfully accepted governance mode change.
            PERMIT();
        }
        else if (op_type == OP_DUD_HOST_REPORT)
        {
            HOST_ADDR_KEY(event_data + DUD_HOST_CANDID_ADDRESS_OFFSET);
            // Check for registration entry.

            // ASSERT_FAILURE_MSG >> This host is not registered.
            ASSERT(state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) != DOESNT_EXIST);

            TOKEN_ID_KEY((uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET)); // Generate token id key.
            // Check for token id entry.

            // ASSERT_FAILURE_MSG >> This host is not registered.
            ASSERT(state_foreign(SBUF(token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) != DOESNT_EXIST);

            uint8_t unique_id[HASH_SIZE] = {0};
            GET_DUD_HOST_CANDIDATE_ID((event_data + DUD_HOST_CANDID_ADDRESS_OFFSET), unique_id);

            // ASSERT_FAILURE_MSG >> Generated candidate hash is not matched with provided.
            ASSERT(BUFFER_EQUAL_32(event_data, unique_id));

            CANDIDATE_ID_KEY(unique_id);

            // ASSERT_FAILURE_MSG >> This candidate is already proposed
            ASSERT(state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) == DOESNT_EXIST);

            const uint32_t candidate_idx = (UINT32_FROM_BUF_LE(&governance_info[LAST_CANDIDATE_IDX_OFFSET]) + 1);

            COPY_20BYTES((candidate_id + CANDIDATE_OWNER_ADDRESS_OFFSET), account_field);
            UINT32_TO_BUF_LE(&candidate_id[CANDIDATE_IDX_OFFSET], candidate_idx);
            COPY_8BYTES(candidate_id + CANDIDATE_SHORT_NAME_OFFSET, DUD_HOST_CAND_SHORTNAME);
            UINT64_TO_BUF_LE(&candidate_id[CANDIDATE_CREATED_TIMESTAMP_OFFSET], cur_ledger_timestamp);
            INT64_TO_BUF_LE(&candidate_id[CANDIDATE_PROPOSAL_FEE_OFFSET], proposal_fee);
            UINT32_TO_BUF_LE(&candidate_id[CANDIDATE_POSITIVE_VOTE_COUNT_OFFSET], zero);
            UINT64_TO_BUF_LE(&candidate_id[CANDIDATE_LAST_VOTE_TIMESTAMP_OFFSET], zero);
            candidate_id[CANDIDATE_STATUS_OFFSET] = CANDIDATE_REJECTED;
            UINT64_TO_BUF_LE(&candidate_id[CANDIDATE_STATUS_CHANGE_TIMESTAMP_OFFSET], cur_ledger_timestamp);
            candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET] = CANDIDATE_REJECTED;

            // Write to states.
            // ASSERT_FAILURE_MSG >> Could not set state for candidate.
            ASSERT(state_foreign_set(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) >= 0);

            // Update the new candidate index.
            governance_info[LAST_CANDIDATE_IDX_OFFSET] = candidate_idx;

            // ASSERT_FAILURE_MSG >> Could not set governance info state.
            ASSERT(state_foreign_set(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) >= 0);

            // PERMIT_MSG >> Successfully reported the dud host.
            PERMIT();
        }

        if (redirect_op_type == OP_REMOVE)
        {
            const uint8_t *proposal_fee_ptr = &candidate_id[CANDIDATE_PROPOSAL_FEE_OFFSET];
            const int64_t proposal_fee = INT64_FROM_BUF_LE(proposal_fee_ptr);

            // Send the proposal fee to the owner.
            etxn_reserve(1);
            PREPARE_PAYMENT_TRUSTLINE_TX(EVR_TOKEN, issuer_accid, proposal_fee, (uint8_t *)(candidate_id + CANDIDATE_OWNER_ADDRESS_OFFSET));
            uint8_t emithash[32];

            // ASSERT_FAILURE_MSG >> Emitting EVR forward txn failed
            ASSERT(emit(SBUF(emithash), SBUF(PAYMENT_TRUSTLINE)) >= 0);
            trace(SBUF("emit hash: "), SBUF(emithash), 1);

            // Clear the proposal states.
            // ASSERT_FAILURE_MSG >> Could not delete the candidate id state.
            ASSERT(state_foreign_set(0, 0, SBUF(STP_CANDIDATE_ID), FOREIGN_REF) >= 0);

            // Dud host candidate does not have a owner state.
            // ASSERT_FAILURE_MSG >> Could not delete the candidate owner state.
            ASSERT(!(CANDIDATE_TYPE(event_data) == NEW_HOOK_CANDIDATE && (state_foreign_set(0, 0, SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) < 0)));

            // PERMIT-MSG >> Successfully removed candidate proposal.
            PERMIT();
        }

        if (op_type == OP_STATUS_CHANGE)
        {
            // We accept only the status change transaction from hook heartbeat account.

            // ASSERT_FAILURE_MSG >> Status change is only allowed from heartbeat account or the registry account (for orphan candidates).
            ASSERT(BUFFER_EQUAL_20(((origin_op_type == OP_REMOVE_ORPHAN_CANDIDATE) ? registry_accid : heartbeat_accid), account_field));

            const uint8_t candidate_type = CANDIDATE_TYPE(event_data);

            // ASSERT_FAILURE_MSG >> Invalid candidate type.
            ASSERT(candidate_type != 0);

            uint8_t vote_status = *(event_data + HASH_SIZE);

            if (origin_op_type == OP_REMOVE_ORPHAN_CANDIDATE)
            {
                // ASSERT_FAILURE_MSG >> Invalid orphan candidate.
                ASSERT((candidate_type == NEW_HOOK_CANDIDATE && vote_status == CANDIDATE_VETOED));

                // If voting is already completed for this candidate handle accordingly
                if (VOTING_COMPLETED(candidate_id[CANDIDATE_STATUS_OFFSET]))
                    vote_status = candidate_id[CANDIDATE_STATUS_OFFSET];
            }
            // If this is from heartbeat account, Candidate status should be equal to the the status sent.
            else
            {
                // ASSERT_FAILURE_MSG >> Invalid status sent with the hook_params.
                ASSERT(candidate_id[CANDIDATE_STATUS_OFFSET] == vote_status);
            }

            // For each candidate type we treat differently.
            if (candidate_type == NEW_HOOK_CANDIDATE || candidate_type == DUD_HOST_CANDIDATE)
            {
                const uint8_t epoch_count = reward_configuration[EPOCH_COUNT_OFFSET];
                const uint32_t first_epoch_reward_quota = UINT32_FROM_BUF_LE(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET]);
                const uint32_t epoch_reward_amount = UINT32_FROM_BUF_LE(&reward_configuration[EPOCH_REWARD_AMOUNT_OFFSET]);

                const uint8_t *proposal_fee_ptr = &candidate_id[CANDIDATE_PROPOSAL_FEE_OFFSET];
                const int64_t proposal_fee = INT64_FROM_BUF_LE(proposal_fee_ptr);

                // If proposal expired 50% of proposal fee will be rebated to owner.
                const int64_t reward_amount = (vote_status == CANDIDATE_ELECTED) ? 0 : (vote_status == CANDIDATE_EXPIRED) ? float_multiply(proposal_fee, float_set(-1, 5))
                                                                                                                          : proposal_fee;
                const uint64_t cur_moment_start_timestamp = GET_MOMENT_START_INDEX(cur_idx);

                uint8_t emithash[HASH_SIZE];
                if (vote_status == CANDIDATE_VETOED || vote_status == CANDIDATE_EXPIRED)
                {
                    etxn_reserve(reward_amount > 0 ? 2 : 1);

                    if (reward_amount > 0)
                    {
                        ADD_TO_REWARD_POOL(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, reward_amount);
                        // Reading the heartbeat-hook account from states (Not yet set to states)
                        uint8_t heartbeat_hook_accid[ACCOUNT_ID_SIZE];

                        // ASSERT_FAILURE_MSG >> Error getting heartbeat hook address from states.
                        ASSERT(state_foreign(SBUF(heartbeat_hook_accid), SBUF(CONF_HEARTBEAT_ADDR), FOREIGN_REF) >= 0);

                        PREPARE_HEARTBEAT_FUND_PAYMENT_TX(reward_amount, heartbeat_hook_accid, txid);

                        // ASSERT_FAILURE_MSG >> EVR funding to heartbeat hook account failed.
                        ASSERT(emit(SBUF(emithash), SBUF(HEARTBEAT_FUND_PAYMENT)) >= 0);

                        trace(SBUF("emit hash: "), SBUF(emithash), 1);
                    }

                    // Clear the proposal states.
                    // ASSERT_FAILURE_MSG >> Could not delete the candidate id state.
                    ASSERT(state_foreign_set(0, 0, SBUF(STP_CANDIDATE_ID), FOREIGN_REF) >= 0);

                    // Dud host candidate does not have a owner state.
                    // ASSERT_FAILURE_MSG >> Could not delete the candidate owner state.
                    ASSERT(!(candidate_type == NEW_HOOK_CANDIDATE && (state_foreign_set(0, 0, SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) < 0)));
                }
                else if (vote_status == CANDIDATE_ELECTED)
                {
                    etxn_reserve(candidate_type == NEW_HOOK_CANDIDATE ? 3 : 2);

                    if (candidate_type == NEW_HOOK_CANDIDATE)
                    {
                        // Sending hook update trigger transactions to registry and heartbeat hooks.
                        PREPARE_HOOK_UPDATE_PAYMENT_TX(1, heartbeat_accid, event_data);

                        // ASSERT_FAILURE_MSG >> Emitting heartbeat hook update trigger failed.
                        ASSERT(emit(SBUF(emithash), SBUF(HOOK_UPDATE_PAYMENT)) >= 0);

                        PREPARE_HOOK_UPDATE_PAYMENT_TX(1, registry_accid, event_data);

                        // ASSERT_FAILURE_MSG >> Emitting registry hook update trigger failed.
                        ASSERT(emit(SBUF(emithash), SBUF(HOOK_UPDATE_PAYMENT)) >= 0);

                        // Update the governance info state.
                        COPY_32BYTES(&governance_info[ELECTED_PROPOSAL_UNIQUE_ID_OFFSET], event_data);
                        UINT64_TO_BUF_LE(&governance_info[PROPOSAL_ELECTED_TIMESTAMP_OFFSET], cur_moment_start_timestamp);

                        // ASSERT_FAILURE_MSG >> Could not set state for governance info.
                        ASSERT(state_foreign_set(governance_info, GOVERNANCE_INFO_VAL_SIZE, SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) >= 0);
                    }
                    else
                    {
                        PREPARE_DUD_HOST_REMOVE_TX(1, registry_accid, DUD_HOST_REMOVE, (uint8_t *)(STP_CANDIDATE_ID + DUD_HOST_CANDID_ADDRESS_OFFSET));
                        // ASSERT_FAILURE_MSG >> Emitting registry dud host removal trigger failed.
                        ASSERT(emit(SBUF(emithash), SBUF(DUD_HOST_REMOVE_TX)) >= 0);
                    }
                }

                if (vote_status == CANDIDATE_ELECTED || vote_status == CANDIDATE_VETOED || vote_status == CANDIDATE_EXPIRED)
                {
                    const int64_t rebate_amount = float_sum(proposal_fee, float_negate(reward_amount));
                    uint8_t *tx_ptr;
                    uint32_t tx_size;
                    const uint8_t *res_event_data_ptr = ((vote_status == CANDIDATE_VETOED) ? CANDIDATE_VETOED_RES : ((vote_status == CANDIDATE_ELECTED) ? CANDIDATE_ACCEPT_RES : CANDIDATE_EXPIRY_RES));
                    if (float_compare(rebate_amount, float_set(0, 0), COMPARE_GREATER) == 1)
                    {
                        PREPARE_CANDIDATE_REBATE_PAYMENT_TX(rebate_amount, candidate_id, res_event_data_ptr, event_data);
                        tx_ptr = CANDIDATE_REBATE_PAYMENT;
                        tx_size = CANDIDATE_REBATE_PAYMENT_TX_SIZE;
                    }
                    else
                    {
                        PREPARE_CANDIDATE_REBATE_MIN_PAYMENT_TX(1, candidate_id, res_event_data_ptr, event_data);
                        tx_ptr = CANDIDATE_REBATE_MIN_PAYMENT;
                        tx_size = CANDIDATE_REBATE_MIN_PAYMENT_TX_SIZE;
                    }
                    // ASSERT_FAILURE_MSG >> EVR funding to candidate account failed.
                    ASSERT(emit(SBUF(emithash), tx_ptr, tx_size) >= 0);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);
                }

                if (candidate_type == NEW_HOOK_CANDIDATE)
                {
                    PERMIT_M("Evernode: New hook candidate status changed.", vote_status);
                }

                PERMIT_M("Evernode: Dud host candidate status changed.", vote_status);
            }
            else if (candidate_type == PILOTED_MODE_CANDIDATE && vote_status == CANDIDATE_ELECTED)
            {
                // Change governance the mode.
                governance_info[GOVERNANCE_MODE_OFFSET] = PILOTED;
                // ASSERT_FAILURE_MSG >> Could not set state for governance_game info.
                ASSERT(state_foreign_set(governance_info, GOVERNANCE_INFO_VAL_SIZE, SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) >= 0);

                // Clear piloted mode candidate if piloted mode is elected.

                // ASSERT_FAILURE_MSG >> Could not set state for piloted mode candidate.
                ASSERT(state_foreign_set(0, 0, SBUF(STP_CANDIDATE_ID), FOREIGN_REF) >= 0);

                PERMIT_M("Evernode: Piloted mode candidate status changed.", vote_status);
            }
        }

        if (redirect_op_type == OP_CHANGE_CONFIGURATION)
        {
            ///////////////////////////////////////////////////////////////
            // Begin : Moment size transition implementation.
            // Do the moment size transition. If new moment size is specified.
            // Set Moment transition info with the configured value;
            if (NEW_MOMENT_SIZE > 0 && moment_size != NEW_MOMENT_SIZE)
            {
                // <transition_index(uint64_t)><moment_size(uint16_t)><index_type(uint8_t)>
                uint8_t moment_transition_info[MOMENT_TRANSIT_INFO_VAL_SIZE] = {0};
                int transition_state_res = state_foreign(SBUF(moment_transition_info), SBUF(CONF_MOMENT_TRANSIT_INFO), FOREIGN_REF);

                // ASSERT_FAILURE_MSG >> Error getting moment size transaction info state.
                ASSERT(!(transition_state_res < 0 && transition_state_res != DOESNT_EXIST));

                // ASSERT_FAILURE_MSG >> There is an already scheduled moment size transition.
                ASSERT(IS_MOMENT_TRANSIT_INFO_EMPTY(moment_transition_info));

                const uint64_t next_moment_start_idx = GET_NEXT_MOMENT_START_INDEX(cur_idx);

                UINT64_TO_BUF_LE(&moment_transition_info[TRANSIT_IDX_OFFSET], next_moment_start_idx);
                UINT16_TO_BUF_LE(&moment_transition_info[TRANSIT_MOMENT_SIZE_OFFSET], NEW_MOMENT_SIZE);
                moment_transition_info[TRANSIT_MOMENT_TYPE_OFFSET] = NEW_MOMENT_TYPE;

                // ASSERT_FAILURE_MSG >> Could not set state for moment transition info.
                ASSERT(state_foreign_set(moment_transition_info, MOMENT_TRANSIT_INFO_VAL_SIZE, SBUF(CONF_MOMENT_TRANSIT_INFO), FOREIGN_REF) >= 0);
            }
            // End : Moment size transition implementation.
            ///////////////////////////////////////////////////////////////

            SET_UINT_STATE_VALUE(DEF_MINT_LIMIT, CONF_MINT_LIMIT, "Evernode: Could not initialize state for mint limit.");
            SET_UINT_STATE_VALUE(DEF_FIXED_REG_FEE, CONF_FIXED_REG_FEE, "Evernode: Could not initialize state for fixed reg fee.");

            UINT16_TO_BUF_LE(&reward_configuration[ACCUMULATED_REWARD_FREQUENCY_OFFSET], DEF_ACCUMULATED_REWARD_FREQUENCY);

            // ASSERT_FAILURE_MSG >> Could not set state for reward configuration.
            ASSERT(state_foreign_set(reward_configuration, REWARD_CONFIGURATION_VAL_SIZE, SBUF(CONF_REWARD_CONFIGURATION), FOREIGN_REF) >= 0);

            SET_UINT_STATE_VALUE(DEF_HOST_HEARTBEAT_FREQ, CONF_HOST_HEARTBEAT_FREQ, "Evernode: Could not initialize state for heartbeat frequency.");
            SET_UINT_STATE_VALUE(DEF_LEASE_ACQUIRE_WINDOW, CONF_LEASE_ACQUIRE_WINDOW, "Evernode: Could not initialize state for lease acquire window.");
            SET_UINT_STATE_VALUE(DEF_MAX_TOLERABLE_DOWNTIME, CONF_MAX_TOLERABLE_DOWNTIME, "Evernode: Could not initialize maximum tolerable downtime.");
            SET_UINT_STATE_VALUE(DEF_EMIT_FEE_THRESHOLD, CONF_MAX_EMIT_TRX_FEE, "Evernode: Could not initialize maximum transaction fee for an emission.");

            // <busyness_detect_period(uint32_t)><busyness_detect_average(uint16_t)>
            uint8_t network_configuration[NETWORK_CONFIGURATION_VAL_SIZE] = {0};
            UINT32_TO_BUF_LE(&network_configuration[NETWORK_BUSYNESS_DETECT_PERIOD_OFFSET], DEF_NETWORK_BUSYNESS_DETECT_PERIOD);
            UINT16_TO_BUF_LE(&network_configuration[NETWORK_BUSYNESS_DETECT_AVERAGE_OFFSET], DEF_NETWORK_BUSYNESS_DETECT_AVERAGE);
            network_busyness_detect_average = DEF_NETWORK_BUSYNESS_DETECT_AVERAGE;

            // ASSERT_FAILURE_MSG >> Could not set state for network configuration.
            ASSERT(state_foreign_set(SBUF(network_configuration), SBUF(CONF_NETWORK_CONFIGURATION), FOREIGN_REF) >= 0);

            if (fee_base_info_state_res == DOESNT_EXIST)
            {
                UINT32_TO_BUF_LE(&trx_fee_base_info[FEE_BASE_AVG_OFFSET], fee_avg);
                UINT16_TO_BUF_LE(&trx_fee_base_info[FEE_BASE_COUNTER_OFFSET], zero);
                UINT64_TO_BUF_LE(&trx_fee_base_info[FEE_BASE_AVG_CHANGED_IDX_OFFSET], zero);
                UINT32_TO_BUF_LE(&trx_fee_base_info[FEE_BASE_AVG_ACCUMULATOR_OFFSET], zero);

                // ASSERT_FAILURE_MSG >> Could not set state for transaction fee base info.
                ASSERT(state_foreign_set(SBUF(trx_fee_base_info), SBUF(STK_TRX_FEE_BASE_INFO), FOREIGN_REF) >= 0);
            }

            // PERMIT_MSG >> Assign/ Change configurations successfully.
            PERMIT();
        }
    }

    // PERMIT_MSG >> Transaction is not handled.
    PERMIT();

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}