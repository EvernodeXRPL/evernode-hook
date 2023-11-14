#include "heartbeat.h"

/**
 * Only the incoming transactions are handled. Outgoing transactions are accepted directly.
 * Manages frequent evernode operations related to hosts and foundation
 * Handles host activeness tracking (heartbeat), Host rewards, Governance votes by hosts and foundation and Applying self hook updates.
 * Does not have states. Accesses governance states in as foreign.
 * Supported transaction types: Payment(IOU|XRP)
 * Hook Params:
 * Key: 4556520100000000000000000000000000000000000000000000000000000001
 * Value: <Hook account id which contains the states (governor hook)>
 * Transaction Params:
 * Key: 4556520100000000000000000000000000000000000000000000000000000002
 * Value: <Predefined evernode operation identifier>
 * Key: 4556520100000000000000000000000000000000000000000000000000000003
 * Value: <Required data to invoke the respective operation>
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
    ASSERT(account_field_len == 20);

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

    int64_t cur_ledger_timestamp = ledger_last_time() + XRPL_TIMESTAMP_OFFSET;

    // Reading the hook governance account from hook params
    uint8_t state_hook_accid[ACCOUNT_ID_SIZE] = {0};
    // ASSERT_FAILURE_MSG >> Error getting the state hook address from params.
    ASSERT(hook_param(SBUF(state_hook_accid), SBUF(PARAM_STATE_HOOK_KEY)) == ACCOUNT_ID_SIZE);

    // <transition index><transition_moment><index_type>
    uint8_t moment_base_info[MOMENT_BASE_INFO_VAL_SIZE] = {0};

    // ASSERT_FAILURE_MSG >> Could not get moment base info state.
    ASSERT(state_foreign(SBUF(moment_base_info), SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF) >= 0);

    uint64_t moment_base_idx = UINT64_FROM_BUF_LE(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);
    uint32_t prev_transition_moment = UINT32_FROM_BUF_LE(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]);
    // If state does not exist, take the moment type from default constant.
    uint8_t cur_moment_type = moment_base_info[MOMENT_TYPE_OFFSET];
    uint64_t cur_idx = cur_moment_type == TIMESTAMP_MOMENT_TYPE ? cur_ledger_timestamp : cur_ledger_seq;

    // Take the moment size from config.
    uint16_t moment_size = 0;
    GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");

    ///////////////////////////////////////////////////////////////
    /////// Moment transition related logic is handled here ///////

    // <transition_index(uint64_t)><moment_size(uint16_t)><index_type(uint8_t)>
    uint8_t moment_transition_info[MOMENT_TRANSIT_INFO_VAL_SIZE] = {0};
    int transition_state_res = state_foreign(SBUF(moment_transition_info), SBUF(CONF_MOMENT_TRANSIT_INFO), FOREIGN_REF);

    // ASSERT_FAILURE_MSG >> Error getting moment size transaction info state.
    ASSERT(!(transition_state_res < 0 && transition_state_res != DOESNT_EXIST));

    if (transition_state_res >= 0)
    {
        // Begin : Moment size transition implementation.
        // If there is a transition, transition_idx specifies a index value to perform that.
        uint64_t transition_idx = UINT64_FROM_BUF_LE(&moment_transition_info[TRANSIT_IDX_OFFSET]);
        if (transition_idx > 0 && cur_idx >= transition_idx)
        {
            uint8_t transit_moment_type = moment_transition_info[TRANSIT_MOMENT_TYPE_OFFSET];

            // Take the transition moment
            const uint32_t transition_moment = GET_MOMENT(transition_idx);

            // Add new moment size to the state.
            const uint8_t *moment_size_ptr = &moment_transition_info[TRANSIT_MOMENT_SIZE_OFFSET];

            // ASSERT_FAILURE_MSG >> Could not update the state for moment size.
            ASSERT(state_foreign_set(moment_size_ptr, 2, SBUF(CONF_MOMENT_SIZE), FOREIGN_REF) >= 0);

            // Update the moment base info.
            UINT64_TO_BUF_LE(&moment_base_info[MOMENT_BASE_POINT_OFFSET], transition_idx);
            UINT32_TO_BUF_LE(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET], transition_moment);
            moment_base_info[MOMENT_TYPE_OFFSET] = moment_transition_info[TRANSIT_MOMENT_TYPE_OFFSET];

            // ASSERT_FAILURE_MSG >> Could not set state for moment base info.
            ASSERT(state_foreign_set(SBUF(moment_base_info), SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF) >= 0);

            moment_size = UINT16_FROM_BUF_LE(moment_size_ptr);
            // Assign the transition state values with zeros.
            CLEAR_MOMENT_TRANSIT_INFO(moment_transition_info);

            // ASSERT_FAILURE_MSG >> Could not set state for moment transition info.
            ASSERT(state_foreign_set(SBUF(moment_transition_info), SBUF(CONF_MOMENT_TRANSIT_INFO), FOREIGN_REF) >= 0);

            moment_base_idx = transition_idx;
            prev_transition_moment = transition_moment;
            cur_moment_type = moment_base_info[MOMENT_TYPE_OFFSET];
            cur_idx = cur_moment_type == TIMESTAMP_MOMENT_TYPE ? cur_ledger_timestamp : cur_ledger_seq;
        }
        // End : Moment size transition implementation.
    }

    ///////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////
    /////// Transaction fee base related logic handled here ///////

    // <fee_base_avg(uint32_t)><avg_changed_idx(uint64_t)><avg_accumulator(uint32_t)><counter(uint16_t)>
    uint8_t trx_fee_base_info[TRX_FEE_BASE_INFO_VAL_SIZE] = {0};
    // ASSERT_FAILURE_MSG >> Error getting transaction fee base info state.
    ASSERT(state_foreign(SBUF(trx_fee_base_info), SBUF(STK_TRX_FEE_BASE_INFO), FOREIGN_REF) >= 0);

    const int64_t cur_fee_base = fee_base();
    uint32_t fee_avg = UINT32_FROM_BUF_LE(&trx_fee_base_info[FEE_BASE_AVG_OFFSET]);
    uint16_t counter = UINT16_FROM_BUF_LE(&trx_fee_base_info[FEE_BASE_COUNTER_OFFSET]);
    uint64_t avg_changed_idx = UINT64_FROM_BUF_LE(&trx_fee_base_info[FEE_BASE_AVG_CHANGED_IDX_OFFSET]);
    uint32_t avg_accumulator = UINT32_FROM_BUF_LE(&trx_fee_base_info[FEE_BASE_AVG_ACCUMULATOR_OFFSET]);

    // <busyness_detect_period(uint32_t)><busyness_detect_average(uint16_t)>
    uint8_t network_configuration[NETWORK_CONFIGURATION_VAL_SIZE] = {0};
    // ASSERT_FAILURE_MSG >> Error getting network configuration state.
    ASSERT(state_foreign(SBUF(network_configuration), SBUF(CONF_NETWORK_CONFIGURATION), FOREIGN_REF) >= 0);
    const uint32_t network_busyness_detect_period = UINT32_FROM_BUF_LE(&network_configuration[NETWORK_BUSYNESS_DETECT_PERIOD_OFFSET]);
    const uint16_t network_busyness_detect_average = UINT16_FROM_BUF_LE(&network_configuration[NETWORK_BUSYNESS_DETECT_AVERAGE_OFFSET]);

    int fee_base_info_changed = 0;
    // Check whether the current fee base is increased or decreased to detect network changes
    if ((100 * cur_fee_base) > ((100 + network_busyness_detect_average) * fee_avg) ||
        ((100 + network_busyness_detect_average) * cur_fee_base) < (100 * fee_avg))
    {
        if (counter == 0)
            avg_changed_idx = cur_idx;

        avg_accumulator += cur_fee_base;
        counter++;

        // If fee stayed for threshold period adjust the average fee.
        if ((cur_idx - avg_changed_idx) > network_busyness_detect_period)
        {
            fee_avg = avg_accumulator / counter; // Average will be recorded in non xfl format.
            avg_changed_idx = 0;
            avg_accumulator = 0;
            counter = 0;
        }

        UINT32_TO_BUF_LE(&trx_fee_base_info[FEE_BASE_AVG_OFFSET], fee_avg);
        fee_base_info_changed = 1;
    }
    // If current fee base recovered, Reset the fee base info.
    else if (counter != 0 || avg_changed_idx != 0 || avg_accumulator != 0)
    {
        counter = 0;
        avg_changed_idx = 0;
        avg_accumulator = 0;
        fee_base_info_changed = 1;
    }

    if (fee_base_info_changed == 1)
    {
        UINT16_TO_BUF_LE(&trx_fee_base_info[FEE_BASE_COUNTER_OFFSET], counter);
        UINT64_TO_BUF_LE(&trx_fee_base_info[FEE_BASE_AVG_CHANGED_IDX_OFFSET], avg_changed_idx);
        UINT32_TO_BUF_LE(&trx_fee_base_info[FEE_BASE_AVG_ACCUMULATOR_OFFSET], avg_accumulator);

        // ASSERT_FAILURE_MSG >> Could not set state for transaction fee base info.
        ASSERT(state_foreign_set(SBUF(trx_fee_base_info), SBUF(STK_TRX_FEE_BASE_INFO), FOREIGN_REF) >= 0);
    }

    ///////////////////////////////////////////////////////////////

    uint8_t event_type[MAX_EVENT_TYPE_SIZE];
    const int64_t event_type_len = otxn_param(SBUF(event_type), SBUF(PARAM_EVENT_TYPE_KEY));
    if (event_type_len == DOESNT_EXIST)
    {
        // PERMIT_MSG >> Transaction is not handled.
        PERMIT();
    }

    // ASSERT_FAILURE_MSG >> Error getting the event type param.
    ASSERT(!(event_type_len < 0));

    // specifically we're interested in the amount sent
    otxn_slot(1);
    int64_t amt_slot = slot_subfield(1, sfAmount, 0);

    enum OPERATION op_type = OP_NONE;
    enum OPERATION redirect_op_type = OP_NONE;

    // ASSERT_FAILURE_MSG >> Could not slot otxn.sfAmount.
    ASSERT(amt_slot >= 0);

    int64_t is_xrp = slot_type(amt_slot, 1);
    // ASSERT_FAILURE_MSG >> Could not determine sent amount type
    ASSERT(is_xrp >= 0);

    if (is_xrp)
    {
        // Host heartbeat.
        if (EQUAL_HEARTBEAT(event_type, event_type_len))
            op_type = OP_HEARTBEAT;
        // Foundation vote.
        else if (EQUAL_CANDIDATE_VOTE(event_type, event_type_len))
            op_type = OP_VOTE;
        // Hook update trigger.
        else if (EQUAL_HOOK_UPDATE(event_type, event_type_len))
            op_type = OP_HOOK_UPDATE;
    }

    // Pending reward request.
    if (EQUAL_PENDING_REWARDS_REQUEST(event_type, event_type_len))
        op_type = OP_REWARD_REQUEST;

    if (op_type == OP_NONE)
    {
        // PERMIT_MSG >> Transaction is not handled.
        PERMIT();
    }

    // Heartbeat without vote does not have data.
    uint8_t event_data[MAX_EVENT_DATA_SIZE];
    int64_t event_data_len = otxn_param(SBUF(event_data), SBUF(PARAM_EVENT_DATA1_KEY));

    // ASSERT_FAILURE_MSG >> Error getting the event data param.
    ASSERT(!(op_type != OP_HEARTBEAT && event_data_len < 0));

    uint8_t issuer_accid[ACCOUNT_ID_SIZE] = {0};
    uint8_t foundation_accid[ACCOUNT_ID_SIZE] = {0};

    // ASSERT_FAILURE_MSG >> Could not get issuer or foundation account id.
    ASSERT(!(state_foreign(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR), FOREIGN_REF) < 0 || state_foreign(SBUF(foundation_accid), SBUF(CONF_FOUNDATION_ADDR), FOREIGN_REF) < 0));

    uint8_t source_is_foundation = BUFFER_EQUAL_20(foundation_accid, account_field);
    const uint32_t cur_moment = GET_MOMENT(cur_idx);

    uint8_t governance_info[GOVERNANCE_INFO_VAL_SIZE];
    uint8_t governance_configuration[GOVERNANCE_CONFIGURATION_VAL_SIZE] = {0};

    // ASSERT_FAILURE_MSG >> Could not get state governance_game info.
    ASSERT(!((op_type == OP_HEARTBEAT || op_type == OP_VOTE) && (state_foreign(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0 || state_foreign(SBUF(governance_configuration), SBUF(CONF_GOVERNANCE_CONFIGURATION), FOREIGN_REF) < 0)))

    // We should always refer to the previous moment's voter base count, So take it here and keep it global.
    const uint32_t voter_base_count = UINT32_FROM_BUF_LE(&governance_info[VOTER_BASE_COUNT_OFFSET]);

    // <token_id(32)><country_code(2)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)><no_of_total_instances(4)><no_of_active_instances(4)>
    // <last_heartbeat_index(8)><version(3)><registration_timestamp(8)><transfer_flag(1)><last_vote_candidate_idx(4)><last_vote_timestamp(8)><support_vote_sent(1)><host_reputation(1)>
    uint8_t host_addr[HOST_ADDR_VAL_SIZE];
    // <host_address(20)><cpu_model_name(40)><cpu_count(2)><cpu_speed(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><email(40)><accumulated_reward_amount(8)>
    uint8_t token_id[TOKEN_ID_VAL_SIZE];

    int64_t pending_rewards = 0;
    int has_pending_rewards = 0;
    uint8_t *host_accid_ptr;

    if (op_type == OP_HEARTBEAT || op_type == OP_REWARD_REQUEST)
    {
        host_accid_ptr = (uint8_t *)((op_type == OP_HEARTBEAT) ? account_field : (event_data + REWARD_REQ_HOST_PARAM_OFFSET));
        HOST_ADDR_KEY(host_accid_ptr);
        // Check for registration entry.
        // ASSERT_FAILURE_MSG >> This host is not registered.
        ASSERT(state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) != DOESNT_EXIST);

        TOKEN_ID_KEY((uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET)); // Generate token id key.
        // Check for token id entry.
        // ASSERT_FAILURE_MSG >> This host is not registered.
        ASSERT(state_foreign(SBUF(token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) != DOESNT_EXIST);
    }

    // Host heartbeat.
    if (op_type == OP_HEARTBEAT)
    {
        // Check the ownership of the token to this user before proceeding.
        int token_exists;
        IS_REG_TOKEN_EXIST(account_field, (host_addr + HOST_TOKEN_ID_OFFSET), token_exists);

        // ASSERT_FAILURE_MSG >> Registration token does not exist.
        ASSERT(token_exists);

        const uint8_t *heartbeat_ptr = &host_addr[HOST_HEARTBEAT_TIMESTAMP_OFFSET];
        const int64_t last_heartbeat_idx = INT64_FROM_BUF_LE(heartbeat_ptr);

        INT64_TO_BUF_LE(heartbeat_ptr, cur_idx);

        // Take the heartbeat frequency.
        uint16_t heartbeat_freq;
        GET_CONF_VALUE(heartbeat_freq, CONF_HOST_HEARTBEAT_FREQ, "Evernode: Could not get heartbeat frequency.");

        uint32_t last_heartbeat_moment = 0;

        // Skip if already sent a heartbeat in this moment.
        int accept_heartbeat = 0;
        if (last_heartbeat_idx == 0)
        {
            last_heartbeat_moment = 0;
            accept_heartbeat = 1;
        }
        else
        {
            last_heartbeat_moment = GET_MOMENT(last_heartbeat_idx);
            if (cur_moment > last_heartbeat_moment)
                accept_heartbeat = 1;
        }

        const uint8_t host_reputation = host_addr[HOST_REPUTATION_OFFSET];

        // <epoch_count(uint8_t)><first_epoch_reward_quota(uint32_t)><epoch_reward_amount(uint32_t)><reward_start_moment(uint32_t)><accumulated_reward_frequency(uint16_t)><host_reputation_threshold(uint8_t)><host_reputation_threshold(uint8_t)>
        uint8_t reward_configuration[REWARD_CONFIGURATION_VAL_SIZE];

        // ASSERT_FAILURE_MSG >> Could not get reward configuration state.
        ASSERT(state_foreign(SBUF(reward_configuration), SBUF(CONF_REWARD_CONFIGURATION), FOREIGN_REF) >= 0);

        const uint8_t reputation_threshold = reward_configuration[HOST_REPUTATION_THRESHOLD_OFFSET];

        // Allocate for both rewards trx + vote triggers.
        etxn_reserve((accept_heartbeat && host_reputation > reputation_threshold) ? 2 : 1);

        if (accept_heartbeat && host_reputation > reputation_threshold)
        {
            // <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)>
            uint8_t reward_info[REWARD_INFO_VAL_SIZE];

            // ASSERT_FAILURE_MSG >> Could not get reward info state.
            ASSERT(state_foreign(SBUF(reward_info), SBUF(STK_REWARD_INFO), FOREIGN_REF) >= 0);

            const uint8_t epoch_count = reward_configuration[EPOCH_COUNT_OFFSET];
            const uint32_t first_epoch_reward_quota = UINT32_FROM_BUF_LE(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET]);
            const uint32_t epoch_reward_amount = UINT32_FROM_BUF_LE(&reward_configuration[EPOCH_REWARD_AMOUNT_OFFSET]);
            const uint32_t reward_start_moment = UINT32_FROM_BUF_LE(&reward_configuration[REWARD_START_MOMENT_OFFSET]);

            int64_t reward_pool_amount, reward_amount;
            PREPARE_EPOCH_REWARD_INFO(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, 1, reward_pool_amount, reward_amount);

            const uint8_t *accumulated_reward_ptr = &token_id[HOST_ACCUMULATED_REWARD_OFFSET];
            int64_t accumulated_reward = INT64_FROM_BUF_LE(accumulated_reward_ptr);

            // Reward if reward start moment has passed AND if this is not the first heartbeat of the host AND host is active in the previous moment AND
            // the reward quota is not 0.
            if ((reward_start_moment == 0 || cur_moment >= reward_start_moment) &&
                last_heartbeat_moment > 0 && last_heartbeat_moment >= (cur_moment - heartbeat_freq - 1) &&
                (float_compare(reward_amount, float_set(0, 0), COMPARE_GREATER) == 1))
            {
                accumulated_reward = float_sum(accumulated_reward, reward_amount);
                reward_pool_amount = float_sum(reward_pool_amount, float_negate(reward_amount));
                INT64_TO_BUF_LE(&reward_info[EPOCH_POOL_OFFSET], reward_pool_amount);
            }

            // Send the accumulated rewards if there's any.
            const uint16_t accumulated_reward_freq = UINT16_FROM_BUF_LE(&reward_configuration[ACCUMULATED_REWARD_FREQUENCY_OFFSET]);
            if (cur_moment % accumulated_reward_freq == 0 && float_compare(accumulated_reward, float_set(0, 0), COMPARE_GREATER) == 1)
            {
                // Keep pending rewards to sent.
                pending_rewards = accumulated_reward;
                has_pending_rewards = 1;
                // Make the accumulated reward 0 after sending the rewards.
                accumulated_reward = float_set(0, 0);
            }

            INT64_TO_BUF_LE(accumulated_reward_ptr, accumulated_reward);

            // ASSERT_FAILURE_MSG >> Could not set state for host token id.
            ASSERT(state_foreign_set(SBUF(token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) >= 0);

            // ASSERT_FAILURE_MSG >> Could not set state for reward info.
            ASSERT(state_foreign_set(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO), FOREIGN_REF) >= 0);
        }

        const uint32_t min_eligibility_period = UINT32_FROM_BUF_LE(&governance_configuration[ELIGIBILITY_PERIOD_OFFSET]);

        uint8_t eligible_for_governance = 0;
        uint8_t do_rollback = 0;
        VALIDATE_GOVERNANCE_ELIGIBILITY(host_addr, cur_ledger_timestamp, min_eligibility_period, eligible_for_governance, do_rollback);

        // Increase the voter base count if this host haven't send heartbeat before and host is eligible for voting.
        if (accept_heartbeat && eligible_for_governance)
        {
            const uint32_t voter_base_count_moment = GET_MOMENT(UINT64_FROM_BUF_LE(&governance_info[VOTER_BASE_COUNT_CHANGED_TIMESTAMP_OFFSET]));
            // Reset the count if this is a new moment.
            const uint32_t voter_base_count_updated = ((cur_moment > voter_base_count_moment ? 0 : voter_base_count) + 1);
            UINT32_TO_BUF_LE(&governance_info[VOTER_BASE_COUNT_OFFSET], voter_base_count_updated);
            UINT64_TO_BUF_LE(&governance_info[VOTER_BASE_COUNT_CHANGED_TIMESTAMP_OFFSET], cur_ledger_timestamp);

            // ASSERT_FAILURE_MSG >> Could not set state governance_game info.
            ASSERT(state_foreign_set(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) != DOESNT_EXIST);
        }

        // Handle votes if there's a vote.
        if (event_data_len > 0)
        {
            // ASSERT_FAILURE_MSG >> This host is not eligible for voting.
            ASSERT(eligible_for_governance);

            redirect_op_type = OP_VOTE;
        }
        else
        {
            // Update the host address state.
            // If this heartbeat contains vote host_addr will be updated from OP_VOTE section.

            // ASSERT_FAILURE_MSG >> Could not set state for heartbeat.
            ASSERT(state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) >= 0);

            // Accept if there're no pending rewards.
            if (!has_pending_rewards)
            {
                // PERMIT_MSG >> Host heartbeat successful.
                PERMIT();
            }
        }
    }
    else if (op_type == OP_VOTE)
    {
        // ASSERT_FAILURE_MSG >> Only foundation is allowed to vote in governance hook.
        ASSERT(source_is_foundation);

        // Allocate vote triggers.
        etxn_reserve(1);

        redirect_op_type = OP_VOTE;
    }
    else if (op_type == OP_REWARD_REQUEST)
    {
        uint8_t registry_accid[ACCOUNT_ID_SIZE] = {0};
        // ASSERT_FAILURE_MSG >> Could not get registry account id.
        ASSERT(!(state_foreign(SBUF(registry_accid), SBUF(CONF_REGISTRY_ADDR), FOREIGN_REF) < 0));

        // ASSERT_FAILURE_MSG >> Only registry is allowed to send reward requests.
        ASSERT(BUFFER_EQUAL_20(registry_accid, account_field))

        // ASSERT_FAILURE_MSG >> Reward request should include reward reference.
        ASSERT(!IS_BUFFER_EMPTY_20(event_data + REWARD_REQ_HOST_PARAM_OFFSET) && !IS_BUFFER_EMPTY_8(event_data + REWARD_REQ_AMOUNT_PARAM_OFFSET));

        const uint8_t *pending_rewards_ptr = &token_id[HOST_ACCUMULATED_REWARD_OFFSET];
        pending_rewards = INT64_FROM_BUF_LE(pending_rewards_ptr);

        const uint8_t *pending_rewards_sent_ptr = &event_data[REWARD_REQ_AMOUNT_PARAM_OFFSET];
        const int64_t pending_rewards_sent = INT64_FROM_BUF(pending_rewards_sent_ptr);

        // ASSERT_FAILURE_MSG >> Host does not have pending rewards.
        ASSERT(float_compare(pending_rewards, float_set(0, 0), COMPARE_GREATER) == 1 && float_compare(pending_rewards, pending_rewards_sent, COMPARE_EQUAL) == 1);

        has_pending_rewards = 1;

        // Remove the host after pending rewards are determined.
        // ASSERT_FAILURE_MSG >> Could not delete host registration entry.
        ASSERT(!(state_foreign_set(0, 0, SBUF(STP_TOKEN_ID), FOREIGN_REF) < 0 || state_foreign_set(0, 0, SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0));

        // Allocate reward payment.
        etxn_reserve(1);
    }
    // Child hook update trigger.
    else if (op_type == OP_HOOK_UPDATE)
        HANDLE_HOOK_UPDATE(CANDIDATE_HEARTBEAT_HOOK_HASH_OFFSET);

    // Send accumulated rewards on heartbeat and reward request if there's any.
    if ((op_type == OP_HEARTBEAT || op_type == OP_REWARD_REQUEST) && has_pending_rewards)
    {
        PREPARE_REWARD_PAYMENT_TX(pending_rewards, host_accid_ptr);
        uint8_t emithash[HASH_SIZE];
        // ASSERT_FAILURE_MSG >> Emitting txn failed
        ASSERT(emit(SBUF(emithash), SBUF(REWARD_PAYMENT)) >= 0);

        if (redirect_op_type != OP_VOTE)
        {
            if (op_type == OP_HEARTBEAT)
            {
                // PERMIT_MSG >> Successfully accepted host heartbeat..
                PERMIT();
            }

            // PERMIT_MSG >> Successfully accepted reward request..
            PERMIT();
        }
    }

    if (redirect_op_type == OP_VOTE)
    {
        // BEGIN : Apply the given votes for existing candidates.

        // Generate Candidate ID state key.
        CANDIDATE_ID_KEY(event_data);
        // <owner_address(20)><candidate_idx(4)><short_name(20)><created_timestamp(8)><proposal_fee(8)><positive_vote_count(4)><negative_vote_count(4)>
        // <last_vote_timestamp(8)><status(1)><status_change_timestamp(8)><foundation_vote_status(1)>
        uint8_t candidate_id[CANDIDATE_ID_VAL_SIZE];

        // ASSERT_FAILURE_MSG >> VOTE_VALIDATION_ERR - Error getting a candidate for the given id.
        ASSERT(state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) >= 0);

        // As first 20 bytes of "candidate_id" represents owner address.
        CANDIDATE_OWNER_KEY(candidate_id);

        const uint8_t candidate_type = CANDIDATE_TYPE(event_data);

        REQUIRE((candidate_type != 0), "Evernode: VOTE_VALIDATION_ERR - Voting for an invalid candidate type.");

        REQUIRE(!VOTING_COMPLETED(candidate_id[CANDIDATE_STATUS_OFFSET]), "Evernode: VOTE_VALIDATION_ERR - Voting for this candidate is now closed.");

        const uint64_t last_vote_timestamp = UINT64_FROM_BUF_LE(&candidate_id[CANDIDATE_LAST_VOTE_TIMESTAMP_OFFSET]);
        const uint32_t last_vote_moment = GET_MOMENT(last_vote_timestamp);
        uint32_t supported_count = UINT32_FROM_BUF_LE(&candidate_id[CANDIDATE_POSITIVE_VOTE_COUNT_OFFSET]);

        uint8_t status = CANDIDATE_REJECTED;
        const uint64_t last_election_completed_timestamp = UINT64_FROM_BUF_LE(&governance_info[PROPOSAL_ELECTED_TIMESTAMP_OFFSET]);
        const uint8_t governance_mode = governance_info[GOVERNANCE_MODE_OFFSET];
        const uint32_t life_period = UINT32_FROM_BUF_LE(&governance_configuration[CANDIDATE_LIFE_PERIOD_OFFSET]);
        const uint32_t election_period = UINT32_FROM_BUF_LE(&governance_configuration[CANDIDATE_ELECTION_PERIOD_OFFSET]);
        const uint64_t created_timestamp = UINT64_FROM_BUF_LE(&candidate_id[CANDIDATE_CREATED_TIMESTAMP_OFFSET]);
        uint8_t cur_status = candidate_id[CANDIDATE_STATUS_OFFSET];
        uint8_t foundation_vote_status = candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET];
        uint64_t status_changed_timestamp = UINT64_FROM_BUF_LE(&candidate_id[CANDIDATE_STATUS_CHANGE_TIMESTAMP_OFFSET]);

        int is_foundation_driven = (governance_mode == PILOTED && source_is_foundation) ? 1 : 0;

        // If this is piloted mode candidate we treat differently.
        // If there is a recently closed election or if the candidate's life period is passed. This voted candidate will be purged.
        if ((candidate_type != PILOTED_MODE_CANDIDATE) &&
            (last_election_completed_timestamp > created_timestamp || cur_ledger_timestamp - created_timestamp > life_period))
        {
            status = CANDIDATE_PURGED;
        }
        // If this is a new moment, Evaluate the votes and vote statuses. Then reset the votes for the next moment.
        else if (cur_moment > last_vote_moment)
        {
            uint64_t election_timestamp = GET_MOMENT_START_INDEX(cur_ledger_timestamp);
            // If the last vote is received before previous moment means the proposal is rejected in the next moment after last_vote_moment.
            if (cur_moment - 1 > last_vote_moment)
            {
                supported_count = 0;
                election_timestamp = GET_NEXT_MOMENT_START_INDEX(last_vote_timestamp + (2 * moment_size));
            }

            // Piloted: which means the Foundation vote determined the outcome of all Proposals
            // Co-Piloted:  which means no Proposal can succeed unless the Foundation Supports it and it fails if the Foundation opposes it.
            // Auto-Piloted: which means the standard voting rules apply with the Foundation being treated equally with any other Participant.
            if (governance_mode == PILOTED)
                status = foundation_vote_status;
            else
            {
                uint32_t prev_voter_base_count = voter_base_count;
                const uint16_t support_average = UINT16_FROM_BUF_LE(&governance_configuration[CANDIDATE_SUPPORT_AVERAGE_OFFSET]);

                // If auto-piloted standard voting rules applies for foundation.
                if (governance_mode == AUTO_PILOTED)
                {
                    prev_voter_base_count++;
                    if (foundation_vote_status == CANDIDATE_SUPPORTED)
                        supported_count++;
                }

                if (prev_voter_base_count > 0 && ((supported_count * 100) > (prev_voter_base_count * support_average)))
                    status = CANDIDATE_SUPPORTED;

                // In co-piloted mode if foundation is not supported, we do not consider other participants' status.
                if (governance_mode == CO_PILOTED && foundation_vote_status != CANDIDATE_SUPPORTED)
                    status = foundation_vote_status;
            }

            // Update the vote status if changed.
            if (status != cur_status)
            {
                candidate_id[CANDIDATE_STATUS_OFFSET] = status;
                cur_status = status;
                UINT64_TO_BUF_LE(&candidate_id[CANDIDATE_STATUS_CHANGE_TIMESTAMP_OFFSET], election_timestamp);
                status_changed_timestamp = election_timestamp;
            }

            // Update the vote counts.
            supported_count = 0;
        }

        // If the candidate is not purged and it fulfils the time period to get elected.
        if (status != CANDIDATE_PURGED && (cur_ledger_timestamp - status_changed_timestamp) > election_period && cur_status == CANDIDATE_SUPPORTED)
            status = CANDIDATE_ELECTED;

        // Also a candidate will be elected
        // * If it is supported by the foundation in the piloted mode and
        // * If there are no other candidates elected after the creation of that.
        if (is_foundation_driven)
            status = (*(event_data + CANDIDATE_VOTE_VALUE_PARAM_OFFSET) == CANDIDATE_SUPPORTED &&
                      (created_timestamp >= last_election_completed_timestamp))
                         ? CANDIDATE_ELECTED
                         : CANDIDATE_PURGED;

        if ((candidate_type != PILOTED_MODE_CANDIDATE) ? VOTING_COMPLETED(status) : (status == CANDIDATE_ELECTED))
        {
            candidate_id[CANDIDATE_STATUS_OFFSET] = status;

            // Invoke Governor to trigger on this condition.
            uint8_t trigger_memo_data[33];
            COPY_32BYTES(trigger_memo_data, event_data);
            trigger_memo_data[32] = status;

            PREPARE_CANDIDATE_STATUS_CHANGE_MIN_PAYMENT(1, state_hook_accid, trigger_memo_data);
            uint8_t emithash[HASH_SIZE];

            // ASSERT_FAILURE_MSG >> Emitting txn failed
            ASSERT(emit(SBUF(emithash), SBUF(CANDIDATE_STATUS_CHANGE_MIN_PAYMENT_TX)) >= 0);
        }
        else
        {
            uint8_t *last_voted_candidate_idx_ptr, *last_voted_timestamp_ptr;
            const uint32_t candidate_idx = UINT32_FROM_BUF_LE(&candidate_id[CANDIDATE_IDX_OFFSET]);

            if (source_is_foundation)
            {
                last_voted_candidate_idx_ptr = &governance_info[FOUNDATION_LAST_VOTED_CANDIDATE_IDX];
                last_voted_timestamp_ptr = &governance_info[FOUNDATION_LAST_VOTED_TIMESTAMP_OFFSET];
                candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET] = *(event_data + CANDIDATE_VOTE_VALUE_PARAM_OFFSET);
            }
            else
            {
                last_voted_candidate_idx_ptr = &host_addr[HOST_LAST_VOTE_CANDIDATE_IDX_OFFSET];
                last_voted_timestamp_ptr = &host_addr[HOST_LAST_VOTE_TIMESTAMP_OFFSET];
                uint8_t *support_vote_flag_ptr = &host_addr[HOST_SUPPORT_VOTE_FLAG_OFFSET];

                const uint32_t last_vote_candidate_idx = UINT32_FROM_BUF_LE(last_voted_candidate_idx_ptr);
                const uint32_t voted_moment = GET_MOMENT(UINT64_FROM_BUF_LE(last_voted_timestamp_ptr));

                // Change the vote flag in a vote of a new moment.
                if (cur_moment != voted_moment)
                    *support_vote_flag_ptr = 0;

                const uint8_t voted_flag = *support_vote_flag_ptr;

                // If this is a new moment last_vote_candidate_idx needed to be reset. So skip this check.

                // ASSERT_FAILURE_MSG >> Voting for already voted candidate is not allowed.
                ASSERT((cur_moment != voted_moment || candidate_idx > last_vote_candidate_idx));

                // Only one support vote is allowed for new hook candidate per moment.
                if (candidate_type == NEW_HOOK_CANDIDATE)
                {
                    // ASSERT_FAILURE_MSG >> Only one support vote is allowed per moment.
                    ASSERT((cur_moment != voted_moment || *(event_data + CANDIDATE_VOTE_VALUE_PARAM_OFFSET) != CANDIDATE_SUPPORTED || voted_flag != 1));

                    *support_vote_flag_ptr = *(event_data + CANDIDATE_VOTE_VALUE_PARAM_OFFSET) == CANDIDATE_SUPPORTED ? 1 : 0;
                }

                // Increase vote count if this is a vote from a host.
                if (*(event_data + CANDIDATE_VOTE_VALUE_PARAM_OFFSET) == CANDIDATE_SUPPORTED)
                    supported_count++;
            }

            UINT32_TO_BUF_LE(&candidate_id[CANDIDATE_POSITIVE_VOTE_COUNT_OFFSET], supported_count);

            // Update the last vote timestamp.
            UINT64_TO_BUF_LE(&candidate_id[CANDIDATE_LAST_VOTE_TIMESTAMP_OFFSET], cur_ledger_timestamp);

            // Update the last vote info.
            UINT64_TO_BUF_LE(last_voted_timestamp_ptr, cur_ledger_timestamp);
            UINT32_TO_BUF_LE(last_voted_candidate_idx_ptr, candidate_idx);

            if (source_is_foundation)
            {
                // ASSERT_FAILURE_MSG >> Could not set state for governance_game info.
                ASSERT(state_foreign_set(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) >= 0);
            }
            else
            {
                // ASSERT_FAILURE_MSG >> Could not set state for host.
                ASSERT(state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) >= 0);
            }
        }

        // ASSERT_FAILURE_MSG >> Could not set state for candidate id.
        ASSERT(state_foreign_set(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) >= 0);

        // END : Apply the given votes for existing candidates.

        if (source_is_foundation)
            // PERMIT_MSG >> Successfully accepted foundation candidate vote.
            PERMIT();

        // PERMIT_MSG >> Successfully accepted host heartbeat candidate vote..
        PERMIT();
    }

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}