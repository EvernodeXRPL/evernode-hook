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
    if (hook_param(SBUF(meta_params), SBUF(META_PARAMS)) < 0 || meta_params[CHAIN_IDX_PARAM_OFFSET] != 2)
        rollback(SBUF("Evernode: Invalid meta params for chain two."), 1);

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
        rollback(SBUF("Evernode: Could not get memo params for chain two."), 1);

    uint8_t chain_two_params[CHAIN_TWO_PARAMS_SIZE];
    if (hook_param(SBUF(chain_two_params), SBUF(CHAIN_TWO_PARAMS)) < 0)
        rollback(SBUF("Evernode: Could not get params for chain two."), 1);
    uint64_t moment_base_idx = UINT64_FROM_BUF(&chain_two_params[MOMENT_BASE_IDX_PARAM_OFFSET]);
    uint8_t cur_moment_type = chain_two_params[CUR_MOMENT_TYPE_PARAM_OFFSET];
    uint64_t cur_idx = UINT64_FROM_BUF(&chain_two_params[CUR_IDX_PARAM_OFFSET]);
    uint8_t *moment_transition_info = &chain_two_params[MOMENT_TRANSITION_INFO_PARAM_OFFSET];

    // <token_id(32)><country_code(2)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
    // <no_of_total_instances(4)><no_of_active_instances(4)><last_heartbeat_index(8)><version(3)><registration_timestamp(8)>
    uint8_t host_addr[HOST_ADDR_VAL_SIZE];
    // <host_address(20)><cpu_model_name(40)><cpu_count(2)><cpu_speed(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)>
    uint8_t token_id[TOKEN_ID_VAL_SIZE];

    // Common logic for host deregistration, heartbeat, update registration, rebate process and transfer.
    if (op_type == OP_HEARTBEAT)
    {
        HOST_ADDR_KEY(account_field)
        // Check for registration entry.
        if (state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) == DOESNT_EXIST)
            rollback(SBUF("Evernode: This host is not registered."), 1);

        TOKEN_ID_KEY((uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET)); // Generate token id key.
        // Check for token id entry.
        if (state_foreign(SBUF(token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) == DOESNT_EXIST)
            rollback(SBUF("Evernode: This host is not registered."), 1);

        // Verification Memo should exists for these set of transactions.
        uint8_t verify_params[VERIFY_PARAMS_SIZE];
        if (hook_param(SBUF(verify_params), SBUF(VERIFY_PARAMS)) < 0)
            rollback(SBUF("Evernode: Could not get verify params."), 1);

        uint16_t nft_idx = UINT16_FROM_BUF(verify_params + 34);

        // Check the ownership of the NFT to this user before proceeding.
        int nft_exists;
        uint16_t nft_flags;
        IS_REG_NFT_EXIST(account_field, hook_accid, (host_addr + HOST_TOKEN_ID_OFFSET), verify_params, nft_idx, nft_exists);
        if (!nft_exists)
            rollback(SBUF("Evernode: Registration NFT does not exist."), 1);
    }

    if (op_type == OP_INITIALIZE)
    {
        uint8_t *issuer_ptr = memo_params;
        uint8_t *foundation_ptr = memo_params + ACCOUNT_ID_SIZE;

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
        if (state_foreign(SBUF(host_count_buf), SBUF(STK_HOST_COUNT), FOREIGN_REF) != DOESNT_EXIST)
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
            if (state_foreign_set(SBUF(moment_base_info_buf), SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF) < 0)
                rollback(SBUF("Evernode: Could not initialize state for moment base info."), 1);

            SET_UINT_STATE_VALUE(DEF_MOMENT_SIZE, CONF_MOMENT_SIZE, "Evernode: Could not initialize state for moment size.");
            SET_UINT_STATE_VALUE(DEF_HOST_REG_FEE, STK_HOST_REG_FEE, "Evernode: Could not initialize state for reg fee.");
            SET_UINT_STATE_VALUE(DEF_MAX_REG, STK_MAX_REG, "Evernode: Could not initialize state for maximum registrants.");

            if (state_foreign_set(issuer_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_ISSUER_ADDR), FOREIGN_REF) < 0)
                rollback(SBUF("Evernode: Could not set state for issuer account."), 1);

            if (state_foreign_set(foundation_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_FOUNDATION_ADDR), FOREIGN_REF) < 0)
                rollback(SBUF("Evernode: Could not set state for foundation account."), 1);
        }

        // <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)>
        uint8_t reward_info[REWARD_INFO_VAL_SIZE];
        if (state_foreign(SBUF(reward_info), SBUF(STK_REWARD_INFO), FOREIGN_REF) == DOESNT_EXIST)
        {
            uint32_t cur_moment;
            GET_MOMENT(cur_moment, cur_idx);
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

        int64_t purchaser_target_price = float_set(DEF_TARGET_PRICE_E, DEF_TARGET_PRICE_M);
        uint8_t purchaser_target_price_buf[8];
        INT64_TO_BUF(purchaser_target_price_buf, purchaser_target_price);
        if (state_foreign_set(SBUF(purchaser_target_price_buf), SBUF(CONF_PURCHASER_TARGET_PRICE), FOREIGN_REF) < 0)
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

            if (state_foreign_set(moment_transition_info, MOMENT_TRANSIT_INFO_VAL_SIZE, SBUF(CONF_MOMENT_TRANSIT_INFO), FOREIGN_REF) < 0)
                rollback(SBUF("Evernode: Could not set state for moment transition info."), 1);
        }
        // End : Moment size transition implementation.
        ///////////////////////////////////////////////////////////////

        accept(SBUF("Evernode: Initialization successful."), 0);
    }
    else if (op_type == OP_HEARTBEAT)
    {
        const uint8_t *heartbeat_ptr = &host_addr[HOST_HEARTBEAT_LEDGER_IDX_OFFSET];
        const int64_t last_heartbeat_idx = INT64_FROM_BUF(heartbeat_ptr);

        INT64_TO_BUF(heartbeat_ptr, cur_idx);

        if (state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0)
            rollback(SBUF("Evernode: Could not set state for heartbeat."), 1);

        // Take the moment size from config.
        uint16_t moment_size;
        GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get-moment size.");

        // Take the heartbeat frequency.
        uint16_t heartbeat_freq;
        GET_CONF_VALUE(heartbeat_freq, CONF_HOST_HEARTBEAT_FREQ, "Evernode: Could not get heartbeat frequency.");

        uint32_t cur_moment, last_heartbeat_moment = 0;
        GET_MOMENT(cur_moment, cur_idx);

        // Skip if already sent a heartbeat in this moment.
        int accept_heartbeat = 0;
        if (last_heartbeat_idx == 0)
        {
            last_heartbeat_moment = 0;
            accept_heartbeat = 1;
        }
        // TODO : This can be removed when the moment transition is stable.
        else if (moment_base_idx > last_heartbeat_idx)
        {
            last_heartbeat_moment = last_heartbeat_idx / DEF_MOMENT_SIZE;
            accept_heartbeat = 1;
        }
        else
        {
            GET_MOMENT(last_heartbeat_moment, last_heartbeat_idx);
            if (cur_moment > last_heartbeat_moment)
                accept_heartbeat = 1;
        }

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
            PREPARE_EPOCH_REWARD_INFO(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, moment_size, 1, reward_pool_amount, reward_amount);

            // Reward if reward start moment has passed AND if this is not the first heartbeat of the host AND host is active in the previous moment AND
            // the reward quota is not 0.
            if ((reward_start_moment == 0 || cur_moment >= reward_start_moment) &&
                last_heartbeat_moment > 0 && last_heartbeat_moment >= (cur_moment - heartbeat_freq - 1) &&
                (float_compare(reward_amount, float_set(0, 0), COMPARE_GREATER) == 1))
            {
                etxn_reserve(1);
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

        accept(SBUF("Evernode: Host heartbeat successful."), 0);
    }

    accept(SBUF("Evernode: Transaction is not handled in Hook Position 2."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}