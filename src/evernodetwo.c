#include "../lib/hookapi.h"
// #include "../lib/emulatorapi.h"
#include "evernode.h"
#include "statekeys.h"

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
    uint8_t issuer_accid[ACCOUNT_ID_SIZE];

    // Common logic for host deregistration, heartbeat, update registration, rebate process and transfer.
    if (op_type == OP_HOST_DE_REG || op_type == OP_HEARTBEAT || op_type == OP_HOST_UPDATE_REG || op_type == OP_HOST_REBATE || op_type == OP_HOST_TRANSFER || op_type == OP_DEAD_HOST_PRUNE)
    {
        // Generate host account key.
        if (op_type != OP_DEAD_HOST_PRUNE)
        {
            HOST_ADDR_KEY(account_field)
        }
        else
        {
            HOST_ADDR_KEY(memo_params);
        }
        // Check for registration entry.
        if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
            rollback(SBUF("Evernode: This host is not registered."), 1);

        TOKEN_ID_KEY((uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET)); // Generate token id key.
        // Check for token id entry.
        if (state(SBUF(token_id), SBUF(STP_TOKEN_ID)) == DOESNT_EXIST)
            rollback(SBUF("Evernode: This host is not registered."), 1);

        // Verification Memo should exists for these set of transactions.
        uint8_t verify_params[VERIFY_PARAMS_SIZE];
        if (hook_param(SBUF(verify_params), SBUF(VERIFY_PARAMS)) < 0)
            rollback(SBUF("Evernode: Could not get verify params."), 1);

        // Obtain NFT Page Keylet and the index of the NFT.
        uint8_t nft_page_keylet[34];
        COPY_32BYTES(nft_page_keylet, verify_params);
        COPY_2BYTES(nft_page_keylet + 32, verify_params + 32);

        uint16_t nft_idx = UINT16_FROM_BUF(verify_params + 34);

        // Check the ownership of the NFT to this user before proceeding.
        int nft_exists;
        IS_REG_NFT_EXIST(nft_page_keylet, nft_idx, nft_exists);
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
        int is_valid = 0;
        EQUAL_20BYTES(is_valid, initializer_accid, account_field);
        if (!is_valid)
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
            int is_empty = 0;
            IS_MOMENT_TRANSIT_INFO_EMPTY(is_empty, moment_transition_info);
            if (!is_empty)
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
    else if (op_type == OP_HOST_DE_REG)
    {
        int is_token_match = 0;
        EQUAL_32BYTES(is_token_match, memo_params, (host_addr + HOST_TOKEN_ID_OFFSET));

        if (!is_token_match)
            rollback(SBUF("Evernode: Token id sent doesn't match with the registered NFT."), 1);

        // Delete registration entries.
        if (state_set(0, 0, SBUF(STP_TOKEN_ID)) < 0 || state_set(0, 0, SBUF(STP_HOST_ADDR)) < 0)
            rollback(SBUF("Evernode: Could not delete host registration entry."), 1);

        // Reduce host count by 1.
        uint64_t host_count;
        GET_HOST_COUNT(host_count);
        host_count -= 1;
        SET_HOST_COUNT(host_count);

        uint64_t reg_fee = UINT64_FROM_BUF(host_addr + HOST_REG_FEE_OFFSET);
        uint64_t fixed_reg_fee;
        GET_CONF_VALUE(fixed_reg_fee, CONF_FIXED_REG_FEE, "Evernode: Could not get fixed reg fee.");

        uint8_t issuer_accid[ACCOUNT_ID_SIZE];
        uint8_t foundation_accid[ACCOUNT_ID_SIZE];
        if (state(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR)) < 0 || state(SBUF(foundation_accid), SBUF(CONF_FOUNDATION_ADDR)) < 0)
            rollback(SBUF("Evernode: Could not get issuer or foundation account id."), 1);

        int64_t amount_half = reg_fee > fixed_reg_fee ? reg_fee / 2 : 0;

        if (reg_fee > fixed_reg_fee)
        {
            // Take the moment size from config.
            uint16_t moment_size;
            GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");

            // <epoch_count(uint8_t)><first_epoch_reward_quota(uint32_t)><epoch_reward_amount(uint32_t)><reward_start_moment(uint32_t)>
            uint8_t reward_configuration[REWARD_CONFIGURATION_VAL_SIZE];
            if (state(reward_configuration, REWARD_CONFIGURATION_VAL_SIZE, SBUF(CONF_REWARD_CONFIGURATION)) < 0)
                rollback(SBUF("Evernode: Could not get reward configuration state."), 1);

            // <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)>
            uint8_t reward_info[REWARD_INFO_VAL_SIZE];
            if (state(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO)) < 0)
                rollback(SBUF("Evernode: Could not get reward info state."), 1);

            const uint8_t epoch_count = reward_configuration[EPOCH_COUNT_OFFSET];
            const uint32_t first_epoch_reward_quota = UINT32_FROM_BUF(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET]);
            const uint32_t epoch_reward_amount = UINT32_FROM_BUF(&reward_configuration[EPOCH_REWARD_AMOUNT_OFFSET]);

            // Add amount_half - 5 to the epoch's reward pool.
            const uint8_t *pool_ptr = &reward_info[EPOCH_POOL_OFFSET];
            INT64_TO_BUF(pool_ptr, float_sum(INT64_FROM_BUF(pool_ptr), float_set(0, amount_half - 5)));
            // Prepare reward info to update host counts and epoch.
            int64_t reward_pool_amount, reward_amount;
            PREPARE_EPOCH_REWARD_INFO(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, moment_size, 0, reward_pool_amount, reward_amount);

            if (state_set(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO)) < 0)
                rollback(SBUF("Evernode: Could not set state for reward info."), 1);
        }

        // Sending nft buy offer to the host.
        etxn_reserve(1);

        uint8_t amt_out[AMOUNT_BUF_SIZE];
        SET_AMOUNT_OUT(amt_out, EVR_TOKEN, issuer_accid, float_set(0, amount_half));
        // Creating the NFT buying offer. If he has paid more than fixed reg fee, we create buy offer to reg_fee/2. If not, for 0 EVR.
        uint8_t buy_tx_buf[PREPARE_NFT_BUY_OFFER_TRUSTLINE_SIZE];
        PREPARE_NFT_BUY_OFFER_TRUSTLINE(buy_tx_buf, amt_out, account_field, (uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET));
        uint8_t emithash[HASH_SIZE];
        if (emit(SBUF(emithash), SBUF(buy_tx_buf)) < 0)
            rollback(SBUF("Evernode: Emitting buying offer to NFT failed."), 1);

        accept(SBUF("Evernode: Host de-registration successful."), 0);
    }
    else if (op_type == OP_HEARTBEAT)
    {
        const uint8_t *heartbeat_ptr = &host_addr[HOST_HEARTBEAT_LEDGER_IDX_OFFSET];
        const int64_t last_heartbeat_idx = INT64_FROM_BUF(heartbeat_ptr);

        INT64_TO_BUF(heartbeat_ptr, cur_idx);

        if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
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
            if (state(reward_configuration, REWARD_CONFIGURATION_VAL_SIZE, SBUF(CONF_REWARD_CONFIGURATION)) < 0)
                rollback(SBUF("Evernode: Could not get reward configuration state."), 1);

            // <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)>
            uint8_t reward_info[REWARD_INFO_VAL_SIZE];
            if (state(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO)) < 0)
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
                uint8_t issuer_accid[20];
                if (state(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR)) < 0)
                    rollback(SBUF("Evernode: Could not get issuer address state."), 1);

                uint8_t amt_out[AMOUNT_BUF_SIZE];
                SET_AMOUNT_OUT(amt_out, EVR_TOKEN, issuer_accid, reward_amount);

                etxn_reserve(1);
                uint8_t txn_out[PREPARE_PAYMENT_HOST_REWARD_SIZE];
                PREPARE_PAYMENT_HOST_REWARD(txn_out, amt_out, 0, account_field);

                uint8_t emithash[HASH_SIZE];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting txn failed"), 1);
                trace(SBUF("emit hash: "), SBUF(emithash), 1);

                INT64_TO_BUF(&reward_info[EPOCH_POOL_OFFSET], float_sum(reward_pool_amount, float_negate(reward_amount)));
            }

            if (state_set(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO)) < 0)
                rollback(SBUF("Evernode: Could not set state for reward info."), 1);
        }

        accept(SBUF("Evernode: Host heartbeat successful."), 0);
    }
    else if (op_type == OP_HOST_UPDATE_REG)
    {
        // Msg format.
        // <token_id(32)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><total_instance_count(4)><active_instances(4)><description(26)><version(3)>
        // All data fields are optional in update info transaction. Update state only if an information update is detected.
        int is_empty = 0;
        int is_updated = 0;

        IS_4BYTES_EMPTY(is_empty, (memo_params + HOST_UPDATE_CPU_MICROSEC_MEMO_OFFSET));
        if (!is_empty)
        {
            COPY_4BYTES((token_id + HOST_CPU_MICROSEC_OFFSET), (memo_params + HOST_UPDATE_CPU_MICROSEC_MEMO_OFFSET));
            is_updated = 1;
        }

        IS_4BYTES_EMPTY(is_empty, (memo_params + HOST_UPDATE_RAM_MB_MEMO_OFFSET));
        if (!is_empty)
        {
            COPY_4BYTES((token_id + HOST_RAM_MB_OFFSET), (memo_params + HOST_UPDATE_RAM_MB_MEMO_OFFSET));
            is_updated = 1;
        }

        IS_4BYTES_EMPTY(is_empty, (memo_params + HOST_UPDATE_DISK_MB_MEMO_OFFSET));
        if (!is_empty)
        {
            COPY_4BYTES((token_id + HOST_DISK_MB_OFFSET), (memo_params + HOST_UPDATE_DISK_MB_MEMO_OFFSET));
            is_updated = 1;
        }

        IS_4BYTES_EMPTY(is_empty, (memo_params + HOST_UPDATE_TOT_INS_COUNT_MEMO_OFFSET));
        if (!is_empty)
        {
            COPY_4BYTES((host_addr + HOST_TOT_INS_COUNT_OFFSET), (memo_params + HOST_UPDATE_TOT_INS_COUNT_MEMO_OFFSET));
            is_updated = 1;
        }

        IS_4BYTES_EMPTY(is_empty, (memo_params + HOST_UPDATE_ACT_INS_COUNT_MEMO_OFFSET));
        if (!is_empty)
        {
            COPY_4BYTES((host_addr + HOST_ACT_INS_COUNT_OFFSET), (memo_params + HOST_UPDATE_ACT_INS_COUNT_MEMO_OFFSET));
            is_updated = 1;
        }

        IS_VERSION_EMPTY(is_empty, (memo_params + HOST_UPDATE_VERSION_MEMO_OFFSET));
        if (!is_empty)
        {
            COPY_BYTE((host_addr + HOST_VERSION_OFFSET), (memo_params + HOST_UPDATE_VERSION_MEMO_OFFSET));
            COPY_BYTE((host_addr + HOST_VERSION_OFFSET + 1), (memo_params + HOST_UPDATE_VERSION_MEMO_OFFSET + 1));
            COPY_BYTE((host_addr + HOST_VERSION_OFFSET + 2), (memo_params + HOST_UPDATE_VERSION_MEMO_OFFSET + 2));
            is_updated = 1;
        }

        if (is_updated)
        {
            if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0 || state_set(SBUF(token_id), SBUF(STP_TOKEN_ID)) < 0)
                rollback(SBUF("Evernode: Could not set state for info update."), 1);
        }

        accept(SBUF("Evernode: Update host info successful."), 0);
    }
    else if (op_type == OP_DEAD_HOST_PRUNE)
    {
        HOST_ADDR_KEY(memo_params); // Generate host account key.
        // Check for registration entry.
        uint8_t reg_entry_buf[HOST_ADDR_VAL_SIZE];
        if (state(SBUF(reg_entry_buf), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
            rollback(SBUF("Evernode: This host is not registered."), 1);

        // Take the moment size from config.
        uint16_t moment_size;
        GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");

        uint8_t *last_active_idx_ptr = &reg_entry_buf[HOST_HEARTBEAT_LEDGER_IDX_OFFSET];
        int64_t last_active_idx = INT64_FROM_BUF(last_active_idx_ptr);
        // If host haven't sent a heartbeat yet, take the registration ledger as the last active ledger.
        if (last_active_idx == 0)
        {
            uint8_t *reg_timestamp_ptr = &reg_entry_buf[HOST_REG_TIMESTAMP_OFFSET];
            uint64_t registration_timestamp = UINT64_FROM_BUF(reg_timestamp_ptr);

            // TODO : Revisit once the transition is stable.
            if (registration_timestamp > 0 && (cur_moment_type == TIMESTAMP_MOMENT_TYPE))
                last_active_idx = registration_timestamp;
            else
            {
                uint8_t *reg_ledger_ptr = &reg_entry_buf[HOST_REG_LEDGER_OFFSET];
                uint64_t reg_ledger = UINT64_FROM_BUF(reg_ledger_ptr);
                // Assumption : One ledger lasts 3 seconds.
                last_active_idx = (cur_moment_type == TIMESTAMP_MOMENT_TYPE) ? cur_ledger_timestamp - (cur_ledger_seq - reg_ledger) * 3 : reg_ledger;
            }
        }
        const int64_t heartbeat_delay = (cur_idx - last_active_idx) / moment_size;

        // Take the maximun tolerable downtime from config.
        uint16_t max_tolerable_downtime;
        GET_CONF_VALUE(max_tolerable_downtime, CONF_MAX_TOLERABLE_DOWNTIME, "Evernode: Could not get the maximum tolerable downtime from the state.");

        if (heartbeat_delay < max_tolerable_downtime)
            rollback(SBUF("Evernode: This host is not eligible for forceful removal based on inactiveness."), 1);

        TOKEN_ID_KEY((uint8_t *)(reg_entry_buf + HOST_TOKEN_ID_OFFSET)); // Generate token id key.

        // Check the ownership of the NFT to this user before proceeding.
        int nft_exists;
        uint8_t issuer[ACCOUNT_ID_SIZE], uri[REG_NFT_URI_SIZE], uri_len;
        uint32_t taxon, nft_seq;
        uint16_t flags, tffee;
        uint8_t *token_id_ptr = &reg_entry_buf[HOST_TOKEN_ID_OFFSET];

        uint8_t emithash[HASH_SIZE];
        uint8_t index_for_burnability = 0;
        if ((flags & (1 << index_for_burnability)) != 0)
        {
            // Reserve for two transaction emissions.
            etxn_reserve(2);

            // Burn Registration NFT.
            uint8_t txn_out[PREPARE_NFT_BURN_SIZE];
            PREPARE_NFT_BURN(txn_out, token_id_ptr, memo_params);

            if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                rollback(SBUF("Evernode: Emitting NFT burn txn failed"), 1);
            trace(SBUF("emit hash: "), SBUF(emithash), 1);
        }
        else
        {
            // Reserve for a transaction emission.
            etxn_reserve(1);
        }

        // Delete registration entries.
        if (state_set(0, 0, SBUF(STP_TOKEN_ID)) < 0 || state_set(0, 0, SBUF(STP_HOST_ADDR)) < 0)
            rollback(SBUF("Evernode: Could not delete host registration entry."), 1);

        // Reduce host count by 1.
        uint64_t host_count;
        GET_HOST_COUNT(host_count);
        host_count -= 1;
        SET_HOST_COUNT(host_count);

        const uint64_t reg_fee = UINT64_FROM_BUF(reg_entry_buf + HOST_REG_FEE_OFFSET);
        uint64_t fixed_reg_fee;
        GET_CONF_VALUE(fixed_reg_fee, CONF_FIXED_REG_FEE, "Evernode: Could not get fixed reg fee.");

        uint8_t issuer_accid[20];
        if (state(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR)) < 0)
            rollback(SBUF("Evernode: Could not get issuer account id."), 1);

        if (reg_fee > fixed_reg_fee)
        {
            uint8_t tx_buf[PREPARE_PAYMENT_PRUNED_HOST_REBATE_SIZE];

            // Sending 50% reg fee to Host account and to the epoch Reward pool.
            uint8_t amt_out_return[AMOUNT_BUF_SIZE];
            const int64_t amount_half = reg_fee / 2;

            SET_AMOUNT_OUT(amt_out_return, EVR_TOKEN, issuer_accid, float_set(0, amount_half));

            // Prepare transaction to send 50% of reg fee to host account.
            PREPARE_PAYMENT_PRUNED_HOST_REBATE(tx_buf, amt_out_return, 0, memo_params);
            if (emit(SBUF(emithash), SBUF(tx_buf)) < 0)
                rollback(SBUF("Evernode: Rebating 1/2 reg fee to host account failed."), 1);
            trace(SBUF("emit hash: "), SBUF(emithash), 1);

            // BEGIN: Update the epoch Reward pool.

            // <epoch_count(uint8_t)><first_epoch_reward_quota(uint32_t)><epoch_reward_amount(uint32_t)><reward_start_moment(uint32_t)>
            uint8_t reward_configuration[REWARD_CONFIGURATION_VAL_SIZE];
            if (state(reward_configuration, REWARD_CONFIGURATION_VAL_SIZE, SBUF(CONF_REWARD_CONFIGURATION)) < 0)
                rollback(SBUF("Evernode: Could not get reward configuration state."), 1);

            // <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)>
            uint8_t reward_info[REWARD_INFO_VAL_SIZE];
            if (state(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO)) < 0)
                rollback(SBUF("Evernode: Could not get reward info state."), 1);

            const uint8_t epoch_count = reward_configuration[EPOCH_COUNT_OFFSET];
            const uint32_t first_epoch_reward_quota = UINT32_FROM_BUF(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET]);
            const uint32_t epoch_reward_amount = UINT32_FROM_BUF(&reward_configuration[EPOCH_REWARD_AMOUNT_OFFSET]);

            // Add amount_half - 5 to the epoch's reward pool.
            const uint8_t *pool_ptr = &reward_info[EPOCH_POOL_OFFSET];
            INT64_TO_BUF(pool_ptr, float_sum(INT64_FROM_BUF(pool_ptr), float_set(0, amount_half - 5)));
            // Prepare reward info to update host counts and epoch.
            int64_t reward_pool_amount, reward_amount;
            PREPARE_EPOCH_REWARD_INFO(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, moment_size, 0, reward_pool_amount, reward_amount);

            if (state_set(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO)) < 0)
                rollback(SBUF("Evernode: Could not set state for reward info."), 1);
        }
        else
        {
            uint8_t tx_buf[PREPARE_MIN_PAYMENT_PRUNED_HOST_SIZE];
            // Prepare MIN XRP transaction to host about pruning.
            PREPARE_MIN_PAYMENT_PRUNED_HOST(tx_buf, 1, memo_params);

            if (emit(SBUF(emithash), SBUF(tx_buf)) < 0)
                rollback(SBUF("Evernode: Minimum XRP to host account failed."), 1);
            trace(SBUF("emit hash: "), SBUF(emithash), 1);
        }

        accept(SBUF("Evernode: Dead Host Pruning successful."), 0);
    }
    else if (op_type == OP_HOST_REBATE)
    {
        uint64_t reg_fee = UINT64_FROM_BUF(&host_addr[HOST_REG_FEE_OFFSET]);

        uint64_t host_reg_fee;
        GET_CONF_VALUE(host_reg_fee, STK_HOST_REG_FEE, "Evernode: Could not get host reg fee state.");

        if (reg_fee > host_reg_fee)
        {
            // Reserve for a transaction emission.
            etxn_reserve(1);

            uint8_t amt_out[AMOUNT_BUF_SIZE];
            SET_AMOUNT_OUT(amt_out, EVR_TOKEN, issuer_accid, float_set(0, (reg_fee - host_reg_fee)));

            // Create the outgoing hosting token txn.
            uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
            PREPARE_PAYMENT_SIMPLE_TRUSTLINE(txn_out, amt_out, account_field, 0, 0);

            uint8_t emithash[32];
            if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                rollback(SBUF("Evernode: Emitting EVR rebate txn failed"), 1);
            trace(SBUF("emit hash: "), SBUF(emithash), 1);

            // Updating the current reg fee in the host state.
            UINT64_TO_BUF(&host_addr[HOST_REG_FEE_OFFSET], host_reg_fee);
            if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                rollback(SBUF("Evernode: Could not update host address state."), 1);
        }
        else
            rollback(SBUF("Evernode: No pending rebates for the host."), 1);

        accept(SBUF("Evernode: Host rebate successful."), 0);
    }
    else if (op_type == OP_HOST_TRANSFER)
    {
        // Check for registration entry, if transferee is different from transfer (transferring to a new account).
        int is_host_as_transferee = 0;
        EQUAL_20BYTES(is_host_as_transferee, memo_params, account_field);
        if (is_host_as_transferee == 0)
        {
            HOST_ADDR_KEY(memo_params); // Generate account key for transferee.
            uint8_t reg_entry_buf[HOST_ADDR_VAL_SIZE];
            if (state(SBUF(reg_entry_buf), SBUF(STP_HOST_ADDR)) != DOESNT_EXIST)
                rollback(SBUF("Evernode: New transferee also a registered host."), 1);
        }

        // Check whether this host has an initiated transfer.
        uint8_t host_transfer_flag = host_addr[HOST_TRANSFER_FLAG_OFFSET];
        if (host_transfer_flag == PENDING_TRANSFER)
            rollback(SBUF("Evernode: Host has a pending transfer."), 1);

        // Check whether there is an already initiated transfer for the transferee
        TRANSFEREE_ADDR_KEY(memo_params);
        // <transferring_host_address(20)><registration_ledger(8)><token_id(32)>
        uint8_t transferee_addr[TRANSFEREE_ADDR_VAL_SIZE];

        if (state(SBUF(transferee_addr), SBUF(STP_TRANSFEREE_ADDR)) != DOESNT_EXIST)
            rollback(SBUF("Evernode: There is a previously initiated transfer for this transferee."), 1);

        // Saving the Pending transfer in Hook States.
        COPY_20BYTES((transferee_addr + TRANSFER_HOST_ADDRESS_OFFSET), account_field);
        INT64_TO_BUF(&transferee_addr[TRANSFER_HOST_LEDGER_OFFSET], cur_ledger_seq);
        COPY_32BYTES((transferee_addr + TRANSFER_HOST_TOKEN_ID_OFFSET), (host_addr + HOST_TOKEN_ID_OFFSET));

        if (state_set(SBUF(transferee_addr), SBUF(STP_TRANSFEREE_ADDR)) < 0)
            rollback(SBUF("Evernode: Could not set state for transferee_addr."), 1);

        // Add transfer in progress flag to existing registration record.
        HOST_ADDR_KEY(account_field);
        host_addr[HOST_TRANSFER_FLAG_OFFSET] = TRANSFER_FLAG;

        if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
            rollback(SBUF("Evernode: Could not set state for host_addr."), 1);

        // Sending nft buy offer to the host.
        etxn_reserve(1);

        // Creating the NFT buy offer for 1 XRP drop.
        uint8_t buy_tx_buf[PREPARE_NFT_BUY_OFFER_SIZE];
        PREPARE_NFT_BUY_OFFER(buy_tx_buf, 1, account_field, (uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET));
        uint8_t emithash[HASH_SIZE];

        if (emit(SBUF(emithash), SBUF(buy_tx_buf)) < 0)
            rollback(SBUF("Evernode: Emitting buying offer to NFT failed."), 1);
        trace(SBUF("emit hash: "), SBUF(emithash), 1);

        accept(SBUF("Evernode: Host transfer initiated successfully."), 0);
    }
    else if (op_type == OP_NONE)
    {
    }

    accept(SBUF("Evernode: Transaction is not handled in Hook Position 2."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}