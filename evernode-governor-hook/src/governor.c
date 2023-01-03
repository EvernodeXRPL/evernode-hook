#include "lib/hookapi.h"
// #include "../lib/emulatorapi.h"
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
    int moment_base_info_state_res = state(moment_base_info, MOMENT_BASE_INFO_VAL_SIZE, SBUF(STK_MOMENT_BASE_INFO));
    if (moment_base_info_state_res < 0 && moment_base_info_state_res != DOESNT_EXIST)
        rollback(SBUF("Evernode: Could not get moment base info state."), 1);
    uint64_t moment_base_idx = UINT64_FROM_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);
    uint32_t prev_transition_moment = UINT32_FROM_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]);
    // If state does not exist, take the moment type from default constant.
    uint8_t cur_moment_type = (moment_base_info_state_res == DOESNT_EXIST) ? DEF_MOMENT_TYPE : moment_base_info[MOMENT_TYPE_OFFSET];
    uint64_t cur_idx = cur_moment_type == TIMESTAMP_MOMENT_TYPE ? cur_ledger_timestamp : cur_ledger_seq;

    ///////////////////////////////////////////////////////////////
    /////// Moment transition related logic is handled here ///////

    // <transition_index(uint64_t)><moment_size(uint16_t)><index_type(uint8_t)>
    uint8_t moment_transition_info[MOMENT_TRANSIT_INFO_VAL_SIZE] = {0};
    int transition_state_res = state(moment_transition_info, MOMENT_TRANSIT_INFO_VAL_SIZE, SBUF(CONF_MOMENT_TRANSIT_INFO));
    if (transition_state_res < 0 && transition_state_res != DOESNT_EXIST)
        rollback(SBUF("Evernode: Error getting moment size transaction info state."), 1);

    uint8_t op_type = OP_NONE;

    if (transition_state_res >= 0)
    {
        // Begin : Moment size transition implementation.
        // If there is a transition, transition_idx specifies a index value to perform that.
        uint64_t transition_idx = UINT64_FROM_BUF(&moment_transition_info[TRANSIT_IDX_OFFSET]);
        if (transition_idx > 0 && cur_idx >= transition_idx)
        {
            uint8_t transit_moment_type = moment_transition_info[TRANSIT_MOMENT_TYPE_OFFSET];

            // Take the transition moment
            uint32_t transition_moment;
            GET_MOMENT(transition_moment, transition_idx);

            // Take the transition index.
            // TODO : This can be removed when the moment transition is stable.
            uint32_t converted_transition_idx;
            if (cur_moment_type == transit_moment_type) // Index type hasn't changed, Use the transition index as it is.
                converted_transition_idx = transition_idx;
            else if (cur_moment_type == TIMESTAMP_MOMENT_TYPE) // If transitioning from timestamp to ledgers, Convert transitioning index to ledgers.
            {
                // Time difference.
                const uint64_t diff = cur_idx - transition_idx;
                converted_transition_idx = cur_ledger_seq - (diff / 3);
            }
            else // If transitioning from ledgers to timestamp, Convert transitioning index to timestamp.
            {
                // Ledger difference.
                const uint64_t diff = cur_idx - transition_idx;
                converted_transition_idx = cur_ledger_timestamp - (diff * 3);
            }

            // Add new moment size to the state.
            const uint8_t *moment_size_ptr = &moment_transition_info[TRANSIT_MOMENT_SIZE_OFFSET];
            if (state_set(moment_size_ptr, 2, SBUF(CONF_MOMENT_SIZE)) < 0)
                rollback(SBUF("Evernode: Could not update the state for moment size."), 1);

            // Update the moment base info.
            UINT64_TO_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET], converted_transition_idx);
            UINT32_TO_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET], transition_moment);
            moment_base_info[MOMENT_TYPE_OFFSET] = moment_transition_info[TRANSIT_MOMENT_TYPE_OFFSET];
            if (state_set(SBUF(moment_base_info), SBUF(STK_MOMENT_BASE_INFO)) < 0)
                rollback(SBUF("Evernode: Could not set state for moment base info."), 1);

            // Assign the transition state values with zeros.
            CLEAR_MOMENT_TRANSIT_INFO(moment_transition_info);
            if (state_set(SBUF(moment_transition_info), SBUF(CONF_MOMENT_TRANSIT_INFO)) < 0)
                rollback(SBUF("Evernode: Could not set state for moment transition info."), 1);

            moment_base_idx = UINT64_FROM_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);
            prev_transition_moment = UINT32_FROM_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]);
            cur_moment_type = moment_base_info[MOMENT_TYPE_OFFSET];
            cur_idx = cur_moment_type == TIMESTAMP_MOMENT_TYPE ? cur_ledger_timestamp : cur_ledger_seq;
        }
        // End : Moment size transition implementation.
    }

    ///////////////////////////////////////////////////////////////

    int64_t txn_type = otxn_type();
    if ((reserved == STRONG_HOOK || reserved == AGAIN_HOOK) && (txn_type == ttPAYMENT || txn_type == ttNFT_ACCEPT_OFFER))
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

                if (reserved == STRONG_HOOK && txn_type == ttPAYMENT)
                {
                    if (amt_slot < 0)
                        rollback(SBUF("Evernode: Could not slot otxn.sfAmount"), 1);

                    int64_t is_xrp = slot_type(amt_slot, 1);
                    if (is_xrp < 0)
                        rollback(SBUF("Evernode: Could not determine sent amount type"), 1);

                    if (is_xrp)
                    {
                        // Host initialization.
                        if (EQUAL_INITIALIZE(type_ptr, type_len))
                        {
                            if (!EQUAL_FORMAT_HEX(format_ptr, format_len))
                                rollback(SBUF("Evernode: Memo format should be hex for initialize."), 1);
                            if (data_len != (4 * ACCOUNT_ID_SIZE))
                                rollback(SBUF("Evernode: Memo data should contain foundation cold wallet, registry heartbeat hook and issuer addresses."), 1);
                            op_type = OP_INITIALIZE;

                            uint8_t *issuer_ptr = data_ptr;
                            uint8_t *foundation_ptr = data_ptr + ACCOUNT_ID_SIZE;
                            uint8_t *registry_hook_ptr = data_ptr + (2 * ACCOUNT_ID_SIZE);
                            uint8_t *heartbeat_hook_ptr = data_ptr + (3 * ACCOUNT_ID_SIZE);

                            uint8_t initializer_accid[ACCOUNT_ID_SIZE];
                            const int initializer_accid_len = util_accid(SBUF(initializer_accid), HOOK_INITIALIZER_ADDR, 35);
                            if (initializer_accid_len < ACCOUNT_ID_SIZE)
                                rollback(SBUF("Evernode: Could not convert initializer account id."), 1);

                            // We accept only the init transaction from hook intializer account
                            if (!BUFFER_EQUAL_20(initializer_accid, account_field))
                                rollback(SBUF("Evernode: Only initializer is allowed to initialize state."), 1);

                            // First check if the states are already initialized by checking one state key for existence.
                            int already_initalized = 0; // For Beta test purposes
                            uint8_t host_count_buf[8];
                            if (state(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) != DOESNT_EXIST)
                            {
                                already_initalized = 1;
                                // rollback(SBUF("Evernode: State is already initialized."), 1);
                            }

                            const uint64_t zero = 0;
                            // Initialize the state.
                            if (!already_initalized)
                            {
                                SET_UINT_STATE_VALUE(zero, STK_HOST_COUNT, "Evernode: Could not initialize state for host count.");

                                uint8_t moment_base_info_buf[MOMENT_BASE_INFO_VAL_SIZE] = {0};
                                moment_base_info_buf[MOMENT_TYPE_OFFSET] = DEF_MOMENT_TYPE;
                                if (state_set(SBUF(moment_base_info_buf), SBUF(STK_MOMENT_BASE_INFO)) < 0)
                                    rollback(SBUF("Evernode: Could not initialize state for moment base info."), 1);

                                SET_UINT_STATE_VALUE(DEF_MOMENT_SIZE, CONF_MOMENT_SIZE, "Evernode: Could not initialize state for moment size.");
                                SET_UINT_STATE_VALUE(DEF_HOST_REG_FEE, STK_HOST_REG_FEE, "Evernode: Could not initialize state for reg fee.");
                                SET_UINT_STATE_VALUE(DEF_MAX_REG, STK_MAX_REG, "Evernode: Could not initialize state for maximum registrants.");

                                if (state_set(issuer_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_ISSUER_ADDR)) < 0)
                                    rollback(SBUF("Evernode: Could not set state for issuer account."), 1);

                                if (state_set(foundation_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_FOUNDATION_ADDR)) < 0)
                                    rollback(SBUF("Evernode: Could not set state for foundation account."), 1);

                                if (state_set(heartbeat_hook_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_HEARTBEAT_HOOK_ADDR)) < 0)
                                    rollback(SBUF("Evernode: Could not set state for heartbeat hook account."), 1);

                                if (state_set(registry_hook_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_REGISTRY_HOOK_ADDR)) < 0)
                                    rollback(SBUF("Evernode: Could not set state for registry hook account."), 1);
                            }

                            // <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)>
                            uint8_t reward_info[REWARD_INFO_VAL_SIZE];
                            if (state(SBUF(reward_info), SBUF(STK_REWARD_INFO)) == DOESNT_EXIST)
                            {
                                uint32_t cur_moment;
                                GET_MOMENT(cur_moment, cur_idx);
                                reward_info[EPOCH_OFFSET] = 1;
                                UINT32_TO_BUF(&reward_info[SAVED_MOMENT_OFFSET], cur_moment);
                                UINT32_TO_BUF(&reward_info[PREV_MOMENT_ACTIVE_HOST_COUNT_OFFSET], zero);
                                UINT32_TO_BUF(&reward_info[CUR_MOMENT_ACTIVE_HOST_COUNT_OFFSET], zero);
                                INT64_TO_BUF(&reward_info[EPOCH_POOL_OFFSET], float_set(0, DEF_EPOCH_REWARD_AMOUNT));
                                if (state_set(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO)) < 0)
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
                            if (state_set(reward_configuration, REWARD_CONFIGURATION_VAL_SIZE, SBUF(CONF_REWARD_CONFIGURATION)) < 0)
                                rollback(SBUF("Evernode: Could not set state for reward configuration."), 1);

                            int64_t purchaser_target_price = float_set(DEF_TARGET_PRICE_E, DEF_TARGET_PRICE_M);
                            uint8_t purchaser_target_price_buf[8];
                            INT64_TO_BUF(purchaser_target_price_buf, purchaser_target_price);
                            if (state_set(SBUF(purchaser_target_price_buf), SBUF(CONF_PURCHASER_TARGET_PRICE)) < 0)
                                rollback(SBUF("Evernode: Could not set state for moment community target price."), 1);

                            ///////////////////////////////////////////////////////////////
                            // Begin : Moment size transition implementation.

                            // Take the moment size from config.
                            uint16_t moment_size;
                            GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");

                            // Do the moment size transition. If new moment size is specified.
                            // Set Moment transition info with the configured value;
                            if (NEW_MOMENT_SIZE > 0 && moment_size != NEW_MOMENT_SIZE)
                            {
                                if (!IS_MOMENT_TRANSIT_INFO_EMPTY(moment_transition_info))
                                    rollback(SBUF("Evernode: There is an already scheduled moment size transition."), 1);

                                uint64_t moment_end_idx;
                                GET_MOMENT_END_INDEX(moment_end_idx, cur_idx);

                                UINT64_TO_BUF(moment_transition_info + TRANSIT_IDX_OFFSET, moment_end_idx);
                                UINT16_TO_BUF(moment_transition_info + TRANSIT_MOMENT_SIZE_OFFSET, NEW_MOMENT_SIZE);
                                moment_transition_info[TRANSIT_MOMENT_TYPE_OFFSET] = NEW_MOMENT_TYPE;

                                if (state_set(moment_transition_info, MOMENT_TRANSIT_INFO_VAL_SIZE, SBUF(CONF_MOMENT_TRANSIT_INFO)) < 0)
                                    rollback(SBUF("Evernode: Could not set state for moment transition info."), 1);
                            }
                            // End : Moment size transition implementation.
                            ///////////////////////////////////////////////////////////////

                            accept(SBUF("Evernode: Initialization successful."), 0);
                        }
                    }
                    else
                    {
                        // IOU payment.

                        // Candidate Proposal
                        if (EQUAL_HOST_PROPOSE_CANDIDATE(type_ptr, type_len))
                        {
                            op_type = OP_HOST_PROPOSE_CANDIDATE;
                            accept(SBUF("Evernode: Hook Candidate Proposal was successful."), 0);
                        }
                        else if (EQUAL_HOST_VOTE_CANDIDATE(type_ptr, type_len))
                        {
                            op_type = OP_HOST_VOTE_CANDIDATE;
                            accept(SBUF("Evernode: Hook Candidate voting was successful."), 0);
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
