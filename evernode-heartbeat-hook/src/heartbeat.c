#include "heartbeat.h"

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

    int64_t cur_ledger_timestamp = ledger_last_time() + XRPL_TIMESTAMP_OFFSET;

    // Reading the hook governance account from hook params
    uint8_t state_hook_accid[ACCOUNT_ID_SIZE] = {0};
    if (hook_param(SBUF(state_hook_accid), SBUF(PARAM_STATE_HOOK)) < ACCOUNT_ID_SIZE)
        rollback(SBUF("Evernode: Error getting the state hook address from params."), 1);

    // <transition index><transition_moment><index_type>
    uint8_t moment_base_info[MOMENT_BASE_INFO_VAL_SIZE] = {0};
    if (state_foreign(moment_base_info, MOMENT_BASE_INFO_VAL_SIZE, SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF) < 0)
        rollback(SBUF("Evernode: Could not get moment base info state."), 1);
    uint64_t moment_base_idx = UINT64_FROM_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);
    uint32_t prev_transition_moment = UINT32_FROM_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]);
    // If state does not exist, take the moment type from default constant.
    uint8_t cur_moment_type = moment_base_info[MOMENT_TYPE_OFFSET];
    uint64_t cur_idx = cur_moment_type == TIMESTAMP_MOMENT_TYPE ? cur_ledger_timestamp : cur_ledger_seq;

    // Take the moment size from config.
    uint16_t moment_size = 0;
    GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");

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

        // Accept any outgoing transactions without further processing.
        if (!BUFFER_EQUAL_20(hook_accid, account_field))
        {
            // Memos
            uint8_t memos[MAX_MEMO_SIZE];
            int64_t memos_len = otxn_field(SBUF(memos), sfMemos);

            if (memos_len)
            {
                uint8_t *memo_ptr, *type_ptr, *format_ptr, *data_ptr;
                uint32_t memo_len, type_len, format_len, data_len;
                int64_t memo_lookup = sto_subarray(memos, memos_len, 0);
                GET_MEMO(memo_lookup, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len);

                // specifically we're interested in the amount sent
                int64_t oslot = otxn_slot(0);
                if (oslot < 0)
                    rollback(SBUF("Evernode: Could not slot originating txn."), 1);

                int64_t amt_slot = slot_subfield(oslot, sfAmount, 0);

                uint8_t op_type = OP_NONE;

                if (amt_slot < 0)
                    rollback(SBUF("Evernode: Could not slot otxn.sfAmount"), 1);

                int64_t is_xrp = slot_type(amt_slot, 1);
                if (is_xrp < 0)
                    rollback(SBUF("Evernode: Could not determine sent amount type"), 1);

                if (is_xrp)
                {
                    // Host heartbeat.
                    if (EQUAL_HEARTBEAT(type_ptr, type_len))
                        op_type = OP_HEARTBEAT;
                    // Foundation vote.
                    else if (EQUAL_CANDIDATE_VOTE(type_ptr, type_len))
                        op_type = OP_VOTE;
                    // Hook update trigger.
                    else if (EQUAL_HOOK_UPDATE(type_ptr, type_len))
                        op_type = OP_HOOK_UPDATE;
                }

                if (op_type != OP_NONE)
                {
                    // Memo format validation.
                    if (!EQUAL_FORMAT_HEX(format_ptr, format_len))
                        rollback(SBUF("Evernode: Memo format should be hex."), 1);

                    uint8_t issuer_accid[ACCOUNT_ID_SIZE] = {0};
                    uint8_t foundation_accid[ACCOUNT_ID_SIZE] = {0};
                    if (state_foreign(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR), FOREIGN_REF) < 0 || state_foreign(SBUF(foundation_accid), SBUF(CONF_FOUNDATION_ADDR), FOREIGN_REF) < 0)
                        rollback(SBUF("Evernode: Could not get issuer or foundation account id."), 1);

                    uint8_t source_is_foundation = BUFFER_EQUAL_20(foundation_accid, account_field);
                    const uint32_t cur_moment = GET_MOMENT(cur_idx);

                    uint8_t governance_info[GOVERNANCE_INFO_VAL_SIZE];
                    uint8_t governance_configuration[GOVERNANCE_CONFIGURATION_VAL_SIZE] = {0};
                    if (op_type == OP_HEARTBEAT || op_type == OP_VOTE)
                    {
                        if (state_foreign(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0 || state_foreign(SBUF(governance_configuration), SBUF(CONF_GOVERNANCE_CONFIGURATION), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not get state governance_game info."), 1);
                    }

                    // <token_id(32)><country_code(2)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
                    // <no_of_total_instances(4)><no_of_active_instances(4)><last_heartbeat_index(8)><version(3)><registration_timestamp(8)><transfer_flag(1)><last_vote_candidate_idx(4)>
                    uint8_t host_addr[HOST_ADDR_VAL_SIZE];
                    // Host heartbeat.
                    if (op_type == OP_HEARTBEAT)
                    {
                        // <host_address(20)><cpu_model_name(40)><cpu_count(2)><cpu_speed(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)>
                        uint8_t token_id[TOKEN_ID_VAL_SIZE];

                        HOST_ADDR_KEY(account_field);

                        // Check for registration entry.
                        if (state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) == DOESNT_EXIST)
                            rollback(SBUF("Evernode: This host is not registered."), 1);

                        TOKEN_ID_KEY((uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET)); // Generate token id key.
                        // Check for token id entry.
                        if (state_foreign(SBUF(token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) == DOESNT_EXIST)
                            rollback(SBUF("Evernode: This host is not registered."), 1);

                        // Check the ownership of the token to this user before proceeding.
                        int token_exists;
                        IS_REG_TOKEN_EXIST(account_field, (host_addr + HOST_TOKEN_ID_OFFSET), token_exists);
                        if (!token_exists)
                            rollback(SBUF("Evernode: Registration token does not exist."), 1);

                        const uint8_t *heartbeat_ptr = &host_addr[HOST_HEARTBEAT_TIMESTAMP_OFFSET];
                        const int64_t last_heartbeat_idx = INT64_FROM_BUF(heartbeat_ptr);

                        INT64_TO_BUF(heartbeat_ptr, cur_idx);

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

                        // Allocate for both rewards trx + vote triggers.
                        etxn_reserve(accept_heartbeat ? 2 : 1);

                        if (accept_heartbeat)
                        {
                            // <epoch_count(uint8_t)><first_epoch_reward_quota(uint32_t)><epoch_reward_amount(uint32_t)><reward_start_moment(uint32_t)>
                            uint8_t reward_configuration[REWARD_CONFIGURATION_VAL_SIZE];
                            if (state_foreign(reward_configuration, REWARD_CONFIGURATION_VAL_SIZE, SBUF(CONF_REWARD_CONFIGURATION), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not get reward configuration state."), 1);

                            // <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)>
                            uint8_t reward_info[REWARD_INFO_VAL_SIZE];
                            if (state_foreign(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not get reward info state."), 1);

                            const uint8_t epoch_count = reward_configuration[EPOCH_COUNT_OFFSET];
                            const uint32_t first_epoch_reward_quota = UINT32_FROM_BUF(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET]);
                            const uint32_t epoch_reward_amount = UINT32_FROM_BUF(&reward_configuration[EPOCH_REWARD_AMOUNT_OFFSET]);
                            const uint32_t reward_start_moment = UINT32_FROM_BUF(&reward_configuration[REWARD_START_MOMENT_OFFSET]);

                            int64_t reward_pool_amount, reward_amount;
                            PREPARE_EPOCH_REWARD_INFO(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, 1, reward_pool_amount, reward_amount);

                            // Reward if reward start moment has passed AND if this is not the first heartbeat of the host AND host is active in the previous moment AND
                            // the reward quota is not 0.
                            if ((reward_start_moment == 0 || cur_moment >= reward_start_moment) &&
                                last_heartbeat_moment > 0 && last_heartbeat_moment >= (cur_moment - heartbeat_freq - 1) &&
                                (float_compare(reward_amount, float_set(0, 0), COMPARE_GREATER) == 1))
                            {
                                PREPARE_REWARD_PAYMENT_TX(reward_amount, account_field);
                                uint8_t emithash[HASH_SIZE];
                                if (emit(SBUF(emithash), SBUF(REWARD_PAYMENT)) < 0)
                                    rollback(SBUF("Evernode: Emitting txn failed"), 1);
                                trace(SBUF("emit hash: "), SBUF(emithash), 1);

                                INT64_TO_BUF(&reward_info[EPOCH_POOL_OFFSET], float_sum(reward_pool_amount, float_negate(reward_amount)));
                            }

                            if (state_foreign_set(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for reward info."), 1);
                        }

                        const uint32_t min_eligibility_period = UINT32_FROM_BUF(&governance_configuration[ELIGIBILITY_PERIOD_OFFSET]);
                        if (state_foreign(foundation_accid, ACCOUNT_ID_SIZE, SBUF(CONF_FOUNDATION_ADDR), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not get state for foundation account."), 1);

                        uint8_t eligible_for_governance = 0;
                        uint8_t do_rollback = 0;
                        VALIDATE_GOVERNANCE_ELIGIBILITY(host_addr, cur_ledger_timestamp, min_eligibility_period, eligible_for_governance, do_rollback);

                        // Increase the voter base count if this host haven't send heartbeat before and host is eligible for voting.
                        if (accept_heartbeat && eligible_for_governance)
                        {
                            uint32_t voter_base_count = UINT32_FROM_BUF(&governance_info[VOTER_BASE_COUNT_OFFSET]);
                            const uint32_t voter_base_count_moment = GET_MOMENT(UINT64_FROM_BUF(&governance_info[VOTER_BASE_COUNT_CHANGED_TIMESTAMP_OFFSET]));
                            // Reset the count if this is a new moment.
                            UINT32_TO_BUF(&governance_info[VOTER_BASE_COUNT_OFFSET], (uint32_t)((cur_moment > voter_base_count_moment ? 0 : voter_base_count) + 1));
                            UINT64_TO_BUF(&governance_info[VOTER_BASE_COUNT_CHANGED_TIMESTAMP_OFFSET], cur_ledger_timestamp);
                            if (state_foreign_set(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) == DOESNT_EXIST)
                                rollback(SBUF("Evernode: Could not set state governance_game info."), 1);
                        }

                        // Handle votes if there's a vote.
                        if (data_len > 0)
                        {
                            if (!eligible_for_governance)
                                rollback(SBUF("Evernode: This host is not eligible for voting."), 1);

                            op_type = OP_VOTE;
                        }
                        else
                        {
                            // Update the host address state.
                            // If this heartbeat contains vote host_addr will be updated from OP_VOTE section.
                            if (state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for heartbeat."), 1);

                            accept(SBUF("Evernode: Host heartbeat successful."), 0);
                        }
                    }
                    else if (op_type == OP_VOTE)
                    {
                        if (!source_is_foundation)
                            rollback(SBUF("Evernode: Only foundation is allowed to vote in governance hook."), 1);

                        // Allocate vote triggers.
                        etxn_reserve(1);
                    }

                    if (op_type == OP_VOTE)
                    {
                        // BEGIN : Apply the given votes for existing candidates.

                        // Generate Candidate ID state key.
                        CANDIDATE_ID_KEY(data_ptr);
                        // <owner_address(20)><candidate_idx(4)><short_name(20)><created_timestamp(8)><proposal_fee(8)><positive_vote_count(4)><negative_vote_count(4)>
                        // <last_vote_timestamp(8)><status(1)><status_change_timestamp(8)><foundation_vote_status(1)>
                        uint8_t candidate_id[CANDIDATE_ID_VAL_SIZE];
                        if (state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: VOTE_VALIDATION_ERR - Error getting a candidate for the given id."), 1);

                        // As first 20 bytes of "candidate_id" represents owner address.
                        CANDIDATE_OWNER_KEY(candidate_id);

                        const uint8_t candidate_type = CANDIDATE_TYPE(data_ptr);
                        if (candidate_type == 0)
                            rollback(SBUF("Evernode: VOTE_VALIDATION_ERR - Voting for an invalid candidate type."), 1);

                        if (VOTING_COMPLETED(candidate_id[CANDIDATE_STATUS_OFFSET]))
                            rollback(SBUF("Evernode: VOTE_VALIDATION_ERR - Voting for this candidate is now closed."), 1);

                        uint32_t voter_base_count = UINT32_FROM_BUF(&governance_info[VOTER_BASE_COUNT_OFFSET]);
                        const uint64_t last_vote_timestamp = UINT64_FROM_BUF(&candidate_id[CANDIDATE_LAST_VOTE_TIMESTAMP_OFFSET]);
                        const uint32_t last_vote_moment = GET_MOMENT(last_vote_timestamp);
                        uint32_t supported_count = UINT32_FROM_BUF(&candidate_id[CANDIDATE_POSITIVE_VOTE_COUNT_OFFSET]);

                        uint8_t status = CANDIDATE_REJECTED;
                        const uint64_t last_election_completed_timestamp = UINT64_FROM_BUF(&governance_info[PROPOSAL_ELECTED_TIMESTAMP_OFFSET]);
                        const uint8_t governance_mode = governance_info[GOVERNANCE_MODE_OFFSET];
                        const uint32_t life_period = UINT32_FROM_BUF(&governance_configuration[CANDIDATE_LIFE_PERIOD_OFFSET]);
                        const uint32_t election_period = UINT32_FROM_BUF(&governance_configuration[CANDIDATE_ELECTION_PERIOD_OFFSET]);
                        const uint64_t created_timestamp = UINT64_FROM_BUF(&candidate_id[CANDIDATE_CREATED_TIMESTAMP_OFFSET]);
                        uint8_t cur_status = candidate_id[CANDIDATE_STATUS_OFFSET];
                        uint8_t foundation_vote_status = candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET];
                        uint64_t status_changed_timestamp = UINT64_FROM_BUF(&candidate_id[CANDIDATE_STATUS_CHANGE_TIMESTAMP_OFFSET]);

                        // If this is piloted mode candidate we treat differently, we do not expire or reject event if there's already elected candidate.
                        // If there is a recently closed election. This voted candidate will be vetoed. (If that candidate was created before the election completion timestamp)
                        if ((candidate_type != PILOTED_MODE_CANDIDATE) &&
                            (last_election_completed_timestamp > created_timestamp) &&
                            (last_election_completed_timestamp < (created_timestamp + life_period)))
                        {
                            status = CANDIDATE_VETOED;
                        }
                        // If the candidate is older than the life period destroy the candidate and rebate the half of staked EVRs.
                        else if ((candidate_type != PILOTED_MODE_CANDIDATE) &&
                                 (cur_ledger_timestamp - created_timestamp > life_period))
                        {
                            status = CANDIDATE_EXPIRED;
                        }
                        // If this is a new moment, Evaluate the votes and vote statuses. Then reset the votes for the next moment.
                        else if (cur_moment > last_vote_moment)
                        {
                            uint64_t election_timestamp = GET_MOMENT_START_INDEX(cur_ledger_timestamp);
                            // If the last vote is received before previous moment means the proposal is rejected in the next moment after last_vote_moment.
                            if (cur_moment - 1 > last_vote_moment)
                            {
                                supported_count = 0;
                                foundation_vote_status = CANDIDATE_REJECTED;
                                election_timestamp = GET_NEXT_MOMENT_START_INDEX(last_vote_timestamp + (2 * moment_size));
                            }

                            // Piloted: which means the Foundation vote determined the outcome of all Proposals
                            // Co-Piloted:  which means no Proposal can succeed unless the Foundation Supports it and it fails if the Foundation opposes it.
                            // Auto-Piloted: which means the standard voting rules apply with the Foundation being treated equally with any other Participant.
                            if (governance_mode == PILOTED)
                                status = foundation_vote_status;
                            else
                            {
                                const uint16_t support_average = UINT16_FROM_BUF(&governance_configuration[CANDIDATE_SUPPORT_AVERAGE_OFFSET]);

                                // If auto-piloted standard voting rules applies for foundation.
                                if (governance_mode == AUTO_PILOTED)
                                {
                                    voter_base_count++;
                                    if (foundation_vote_status == CANDIDATE_SUPPORTED)
                                        supported_count++;
                                }

                                if (voter_base_count > 0 && ((supported_count * 100) / voter_base_count > support_average))
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
                                UINT64_TO_BUF(&candidate_id[CANDIDATE_STATUS_CHANGE_TIMESTAMP_OFFSET], election_timestamp);
                                status_changed_timestamp = election_timestamp;
                            }

                            // Reset the foundation vote.
                            candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET] = (uint8_t)CANDIDATE_REJECTED;
                            // Update the vote counts.
                            supported_count = 0;
                        }

                        // If the candidate is not vetoed and expired and it fulfils the time period to get elected or vetoed.
                        if (status != CANDIDATE_VETOED && status != CANDIDATE_EXPIRED && (cur_ledger_timestamp - status_changed_timestamp) > election_period)
                            status = (cur_status == CANDIDATE_SUPPORTED) ? CANDIDATE_ELECTED : CANDIDATE_VETOED;

                        if ((candidate_type != PILOTED_MODE_CANDIDATE) ? VOTING_COMPLETED(status) : (status == CANDIDATE_ELECTED))
                        {
                            candidate_id[CANDIDATE_STATUS_OFFSET] = status;

                            // Invoke Governor to trigger on this condition.
                            uint8_t trigger_memo_data[33];
                            COPY_32BYTES(trigger_memo_data, data_ptr);
                            trigger_memo_data[32] = status;

                            PREPARE_CANDIDATE_STATUS_CHANGE_MIN_PAYMENT(1, state_hook_accid, trigger_memo_data);
                            uint8_t emithash[HASH_SIZE];
                            if (emit(SBUF(emithash), SBUF(CANDIDATE_STATUS_CHANGE_MIN_PAYMENT_TX)) < 0)
                                rollback(SBUF("Evernode: Emitting txn failed"), 1);
                            trace(SBUF("emit hash: "), SBUF(emithash), 1);
                        }
                        else
                        {
                            uint8_t *last_voted_candidate_idx_ptr, *last_voted_timestamp_ptr, *support_vote_flag_ptr;
                            if (source_is_foundation)
                            {
                                last_voted_candidate_idx_ptr = &governance_info[FOUNDATION_LAST_VOTED_CANDIDATE_IDX];
                                last_voted_timestamp_ptr = &governance_info[FOUNDATION_LAST_VOTED_TIMESTAMP_OFFSET];
                                support_vote_flag_ptr = &governance_info[FOUNDATION_SUPPORT_VOTE_FLAG_OFFSET];
                            }
                            else
                            {
                                last_voted_candidate_idx_ptr = &host_addr[HOST_LAST_VOTE_CANDIDATE_IDX_OFFSET];
                                last_voted_timestamp_ptr = &host_addr[HOST_LAST_VOTE_TIMESTAMP_OFFSET];
                                support_vote_flag_ptr = &host_addr[HOST_SUPPORT_VOTE_FLAG_OFFSET];
                            }
                            const uint32_t candidate_idx = UINT32_FROM_BUF(&candidate_id[CANDIDATE_IDX_OFFSET]);
                            const uint32_t last_vote_candidate_idx = UINT32_FROM_BUF(last_voted_candidate_idx_ptr);
                            const uint32_t voted_moment = GET_MOMENT(UINT64_FROM_BUF(last_voted_timestamp_ptr));

                            // Change the vote flag in a vote of a new moment.
                            if (cur_moment != voted_moment)
                                *support_vote_flag_ptr = 0;

                            const uint8_t voted_flag = *support_vote_flag_ptr;

                            // If this is a new moment last_vote_candidate_idx needed to be reset. So skip this check.
                            if (cur_moment == voted_moment && candidate_idx <= last_vote_candidate_idx)
                                rollback(SBUF("Evernode: Voting for already voted candidate is not allowed."), 1);
                            // Only one support vote is allowed for new hook candidate per moment.
                            if (candidate_type == NEW_HOOK_CANDIDATE)
                            {
                                if (cur_moment == voted_moment && *(data_ptr + CANDIDATE_VOTE_VALUE_MEMO_OFFSET) == CANDIDATE_SUPPORTED && voted_flag == 1)
                                    rollback(SBUF("Evernode: Only one support vote is allowed per moment."), 1);

                                *support_vote_flag_ptr = *(data_ptr + CANDIDATE_VOTE_VALUE_MEMO_OFFSET) == CANDIDATE_SUPPORTED ? 1 : 0;
                            }

                            // Increase vote count if this is a vote from a host.
                            if (source_is_foundation)
                                candidate_id[CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET] = *(data_ptr + CANDIDATE_VOTE_VALUE_MEMO_OFFSET);
                            else if (*(data_ptr + CANDIDATE_VOTE_VALUE_MEMO_OFFSET) == CANDIDATE_SUPPORTED)
                                supported_count++;

                            UINT32_TO_BUF(&candidate_id[CANDIDATE_POSITIVE_VOTE_COUNT_OFFSET], supported_count);

                            // Update the last vote timestamp.
                            UINT64_TO_BUF(&candidate_id[CANDIDATE_LAST_VOTE_TIMESTAMP_OFFSET], cur_ledger_timestamp);

                            // Update the last vote info.
                            UINT64_TO_BUF(last_voted_timestamp_ptr, cur_ledger_timestamp);
                            UINT32_TO_BUF(last_voted_candidate_idx_ptr, candidate_idx);
                            if (source_is_foundation)
                            {
                                if (state_foreign_set(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) < 0)
                                    rollback(SBUF("Evernode: Could not set state for governance_game info."), 1);
                            }
                            else
                            {
                                if (state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0)
                                    rollback(SBUF("Evernode: Could not set state for host."), 1);
                            }
                        }

                        if (state_foreign_set(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not set state for candidate id."), 1);

                        // END : Apply the given votes for existing candidates.

                        if (source_is_foundation)
                            accept(SBUF("Evernode: Successfully accepted foundation candidate vote."), 0);
                        else
                            accept(SBUF("Evernode: Successfully accepted host heartbeat candidate vote.."), 0);
                    }
                    // Child hook update trigger.
                    else if (op_type == OP_HOOK_UPDATE)
                        HANDLE_HOOK_UPDATE(CANDIDATE_HEARTBEAT_HOOK_HASH_OFFSET);
                }
            }
        }
    }

    accept(SBUF("Evernode: Transaction is not handled."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}