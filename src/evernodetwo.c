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
    uint8_t common_params[COMMON_CHAIN_PARAMS_SIZE];
    if (hook_param(SBUF(common_params), SBUF(COMMON_CHAIN_PARAMS)) < 0)
        rollback(SBUF("Evernode: Could not get common chain params for chain two."), 1);

    if (common_params[CHAIN_IDX_PARAM_OFFSET] == 2)
    {
        uint8_t *seq_param_ptr = &common_params[CUR_LEDGER_SEQ_PARAM_OFFSET];
        int64_t cur_ledger_seq = INT64_FROM_BUF(seq_param_ptr);
        uint8_t *ts_param_ptr = &common_params[CUR_LEDGER_TIMESTAMP_PARAM_OFFSET];
        int64_t cur_ledger_timestamp = INT64_FROM_BUF(ts_param_ptr);
        unsigned char hook_accid[ACCOUNT_ID_SIZE];
        COPY_BUF(hook_accid, 0, common_params, HOOK_ACCID_PARAM_OFFSET, ACCOUNT_ID_SIZE);
        uint8_t account_field[ACCOUNT_ID_SIZE];
        COPY_BUF(account_field, 0, common_params, ACCOUNT_FIELD_PARAM_OFFSET, ACCOUNT_ID_SIZE);

        uint8_t chain_two_params[CHAIN_TWO_PARAMS_SIZE];
        if (hook_param(SBUF(chain_two_params), SBUF(CHAIN_TWO_PARAMS)) < 0)
            rollback(SBUF("Evernode: Could not get params for chain two."), 1);
        uint64_t moment_base_idx = UINT64_FROM_BUF(&chain_two_params[MOMENT_BASE_IDX_PARAM_OFFSET]);
        uint8_t cur_moment_type = chain_two_params[CUR_MOMENT_TYPE_PARAM_OFFSET];
        uint64_t cur_idx = UINT64_FROM_BUF(&chain_two_params[CUR_IDX_PARAM_OFFSET]);
        uint8_t moment_transition_info[MOMENT_TRANSIT_INFO_VAL_SIZE];
        COPY_BUF(moment_transition_info, 0, chain_two_params, MOMENT_TRANSITION_INFO_PARAM_OFFSET, MOMENT_BASE_INFO_VAL_SIZE);

        // Memos
        uint8_t memos[MAX_MEMO_SIZE];
        int64_t memos_len = otxn_field(SBUF(memos), sfMemos);

        if (!memos_len)
            accept(SBUF("Evernode: No memos found, Unhandled transaction."), 1);

        uint8_t *memo_ptr, *type_ptr, *format_ptr, *data_ptr;
        uint32_t memo_len, type_len, format_len, data_len;
        GET_MEMO(0, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len);

        // Host initialization.
        int is_initialize = 0;
        BUFFER_EQUAL_STR(is_initialize, type_ptr, type_len, INITIALIZE);

        // Host deregistration.
        int is_host_de_reg = 0;
        BUFFER_EQUAL_STR(is_host_de_reg, type_ptr, type_len, HOST_DE_REG);

        // Host heartbeat.
        int is_host_heartbeat = 0;
        BUFFER_EQUAL_STR(is_host_heartbeat, type_ptr, type_len, HEARTBEAT);

        // Host update registration.
        int is_host_update_reg = 0;
        BUFFER_EQUAL_STR(is_host_update_reg, type_ptr, type_len, HOST_UPDATE_REG);

        // Dead Host Prune.
        int is_dead_host_prune = 0;
        BUFFER_EQUAL_STR(is_dead_host_prune, type_ptr, type_len, DEAD_HOST_PRUNE);

        // Host initiate transfer
        int is_host_transfer = 0;
        BUFFER_EQUAL_STR_GUARD(is_host_transfer, type_ptr, type_len, HOST_INIT_TRANSFER, 1);

        // <token_id(32)><country_code(2)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
        // <no_of_total_instances(4)><no_of_active_instances(4)><last_heartbeat_index(8)><version(3)><registration_timestamp(8)>
        uint8_t host_addr[HOST_ADDR_VAL_SIZE];

        // Common logic for host deregistration, heartbeat and update registration.
        if (is_host_de_reg || is_host_heartbeat || is_host_update_reg || is_host_transfer)
        {
            if (is_host_de_reg || is_host_transfer)
            {
                int is_format_hex = 0;
                BUFFER_EQUAL_STR(is_format_hex, format_ptr, format_len, FORMAT_HEX);
                if (!is_format_hex)
                    rollback(SBUF("Evernode: Format should be hex for host deregistration."), 1);
            }
            else if (is_host_update_reg)
            {
                int is_format_plain_text = 0;
                BUFFER_EQUAL_STR_GUARD(is_format_plain_text, format_ptr, format_len, FORMAT_TEXT, 1);
                if (!is_format_plain_text)
                    rollback(SBUF("Evernode: Instance update info format not supported."), 1);
            }

            HOST_ADDR_KEY(account_field); // Generate host account key.
            // Check for registration entry.
            if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                rollback(SBUF("Evernode: This host is not registered."), 1);

            // Check the ownership of the NFT to this user before proceeding.
            int nft_exists;
            uint8_t issuer[20], uri[64], uri_len;
            uint32_t taxon, nft_seq;
            uint16_t flags, tffee;
            uint8_t *token_id_ptr = &host_addr[HOST_TOKEN_ID_OFFSET];
            GET_NFT(account_field, token_id_ptr, nft_exists, issuer, uri, uri_len, taxon, flags, tffee, nft_seq);
            if (!nft_exists)
                rollback(SBUF("Evernode: Token mismatch with registration."), 1);

            // Issuer of the NFT should be the registry contract.
            BUFFER_EQUAL(nft_exists, issuer, hook_accid, ACCOUNT_ID_SIZE);
            if (!nft_exists)
                rollback(SBUF("Evernode: NFT Issuer mismatch with registration."), 1);
        }

        if (is_initialize)
        {
            BUFFER_EQUAL_STR(is_initialize, format_ptr, format_len, FORMAT_HEX);
            if (!is_initialize)
                rollback(SBUF("Evernode: Format should be hex for initialize request."), 1);

            if (data_len != (2 * ACCOUNT_ID_SIZE))
                rollback(SBUF("Evernode: Memo data should contain foundation cold wallet and issuer addresses."), 1);

            uint8_t *issuer_ptr = data_ptr;
            uint8_t *foundation_ptr = data_ptr + ACCOUNT_ID_SIZE;

            uint8_t initializer_accid[ACCOUNT_ID_SIZE];
            const int initializer_accid_len = util_accid(SBUF(initializer_accid), HOOK_INITIALIZER_ADDR, 35);
            if (initializer_accid_len < ACCOUNT_ID_SIZE)
                rollback(SBUF("Evernode: Could not convert initializer account id."), 1);

            // We accept only the init transaction from hook intializer account
            BUFFER_EQUAL(is_initialize, initializer_accid, account_field, ACCOUNT_ID_SIZE);
            if (!is_initialize)
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
                SET_UINT_STATE_VALUE(zero, STK_MOMENT_BASE_INFO, "Evernode: Could not initialize state for moment base info.");
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
            TRACEVAR(moment_size);

            // Do the moment size transition. If new moment size is specified.
            // Set Moment transition info with the configured value;
            if (NEW_MOMENT_SIZE > 0 && moment_size != NEW_MOMENT_SIZE)
            {
                int is_empty = 0;
                IS_BUF_EMPTY(is_empty, moment_transition_info, MOMENT_TRANSIT_INFO_VAL_SIZE);
                if (!is_empty)
                    rollback(SBUF("Evernode: There is an already scheduled moment size transition."), 1);

                uint64_t moment_end_idx;
                GET_MOMENT_END_INDEX(moment_end_idx, cur_idx);

                UINT64_TO_BUF(&moment_transition_info[TRANSIT_IDX_OFFSET], moment_end_idx);
                UINT16_TO_BUF(&moment_transition_info[TRANSIT_MOMENT_SIZE_OFFSET], NEW_MOMENT_SIZE);
                moment_transition_info[TRANSIT_MOMENT_TYPE_OFFSET] = NEW_MOMENT_TYPE;

                if (state_set(moment_transition_info, MOMENT_TRANSIT_INFO_VAL_SIZE, SBUF(CONF_MOMENT_TRANSIT_INFO)) < 0)
                    rollback(SBUF("Evernode: Could not set state for moment transition info."), 1);
            }
            // End : Moment size transition implementation.
            ///////////////////////////////////////////////////////////////

            accept(SBUF("Evernode: Initialization successful."), 0);
        }
        else if (is_host_de_reg)
        {
            int is_token_match = 0;
            BUFFER_EQUAL(is_token_match, data_ptr, host_addr + HOST_TOKEN_ID_OFFSET, NFT_TOKEN_ID_SIZE);

            if (!is_token_match)
                rollback(SBUF("Evernode: Token id sent doesn't match with the registered NFT."), 1);

            TOKEN_ID_KEY((uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET)); // Generate token id key.

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

            uint8_t issuer_accid[20];
            uint8_t foundation_accid[20];
            if (state(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR)) < 0 || state(SBUF(foundation_accid), SBUF(CONF_FOUNDATION_ADDR)) < 0)
                rollback(SBUF("Evernode: Could not get issuer or foundation account id."), 1);

            int64_t amount_half = reg_fee > fixed_reg_fee ? reg_fee / 2 : 0;

            if (reg_fee > fixed_reg_fee)
            {
                // Take the moment size from config.
                uint16_t moment_size;
                GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");
                TRACEVAR(moment_size);

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
        else if (is_host_heartbeat)
        {
            const uint8_t *heartbeat_ptr = &host_addr[HOST_HEARTBEAT_LEDGER_IDX_OFFSET];
            const int64_t last_heartbeat_idx = INT64_FROM_BUF(heartbeat_ptr);

            INT64_TO_BUF(heartbeat_ptr, cur_idx);

            if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                rollback(SBUF("Evernode: Could not set state for heartbeat."), 1);

            // Take the moment size from config.
            uint16_t moment_size;
            GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get-moment size.");
            TRACEVAR(moment_size);

            // Take the heartbeat frequency.
            uint16_t heartbeat_freq;
            GET_CONF_VALUE(heartbeat_freq, CONF_HOST_HEARTBEAT_FREQ, "Evernode: Could not get heartbeat frequency.");
            TRACEVAR(heartbeat_freq);

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
        else if (is_host_update_reg)
        {
            // Msg format.
            // <token_id>;<country_code>;<cpu_microsec>;<ram_mb>;<disk_mb>;<total_instance_count>;<active_instances>;<description>;<version>
            uint32_t section_number = 0, active_instances_len = 0, version_len = 0;
            uint8_t *active_instances_ptr, *version_ptr;
            int info_updated = 0;
            for (int i = 0; GUARD(MAX_MEMO_DATA_LEN), i < data_len; ++i)
            {
                uint8_t *str_ptr = data_ptr + i;
                // Colon means this is an end of the section.
                // If so, we start reading the new section and reset the write index.
                // Stop reading if an empty byte is reached.
                if (*str_ptr == ';')
                {
                    section_number++;
                    continue;
                }
                else if (*str_ptr == 0)
                    break;

                if (section_number == 6) // We only handle active instances for now.
                {
                    if (active_instances_len == 0)
                    {
                        active_instances_ptr = str_ptr;
                        info_updated = 1;
                    }
                    active_instances_len++;
                }
                else if (section_number == 8)
                {
                    if (version_len == 0)
                    {
                        version_ptr = str_ptr;
                        info_updated = 1;
                    }
                    version_len++;
                }
            }

            // All data fields are optional in update info transaction. Update state only if an information update is detected.
            if (info_updated)
            {
                uint32_t active_instances;
                STR_TO_UINT(active_instances, active_instances_ptr, active_instances_len);

                TRACEVAR(active_instances);

                // Populate the values to the buffer.
                UINT32_TO_BUF(host_addr + HOST_ACT_INS_COUNT_OFFSET, active_instances);

                if (version_len > 0) // Version update detected.
                {
                    uint8_t *major_ptr, *minor_ptr, *patch_ptr;
                    uint8_t version_section = 0, major_len = 0, minor_len = 0, patch_len = 0;
                    for (int i = 0; GUARD(MAX_VERSION_LEN), i < version_len; ++i)
                    {
                        uint8_t *str_ptr = version_ptr + i;

                        // Dot means this is an end of the section.
                        // If so, we start reading the new section and reset the write index.
                        // Stop reading if an empty byte is reached.
                        if (*str_ptr == '.')
                        {
                            version_section++;
                            continue;
                        }
                        else if (*str_ptr == 0)
                            break;

                        if (version_section == 0) // Major
                        {
                            if (major_len == 0)
                                major_ptr = str_ptr;

                            major_len++;
                        }
                        else if (version_section == 1) // Minor
                        {
                            if (minor_len == 0)
                                minor_ptr = str_ptr;

                            minor_len++;
                        }
                        else if (version_section == 2) // Patch
                        {
                            if (patch_len == 0)
                                patch_ptr = str_ptr;

                            patch_len++;
                        }
                    }

                    if (major_ptr == 0 || minor_ptr == 0 || patch_ptr == 0)
                        rollback(SBUF("Evernode: Invalid version format."), 1);

                    STR_TO_UINT(host_addr[HOST_VERSION_OFFSET], major_ptr, major_len);
                    STR_TO_UINT(host_addr[HOST_VERSION_OFFSET + 1], minor_ptr, minor_len);
                    STR_TO_UINT(host_addr[HOST_VERSION_OFFSET + 2], patch_ptr, patch_len);
                }

                if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                    rollback(SBUF("Evernode: Could not set state for info update."), 1);
            }

            accept(SBUF("Evernode: Update host info successful."), 0);
        }
        else if (is_dead_host_prune)
        {
            int is_format_hex = 0;
            BUFFER_EQUAL_STR(is_format_hex, format_ptr, format_len, FORMAT_HEX);
            if (!is_format_hex)
                rollback(SBUF("Evernode: Format should be in HEX to prune the host."), 1);

            HOST_ADDR_KEY(data_ptr); // Generate host account key.
            // Check for registration entry.
            uint8_t reg_entry_buf[HOST_ADDR_VAL_SIZE];
            if (state(SBUF(reg_entry_buf), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                rollback(SBUF("Evernode: This host is not registered."), 1);

            // Take the moment size from config.
            uint16_t moment_size;
            GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");
            TRACEVAR(moment_size);

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
            uint8_t issuer[20], uri[64], uri_len;
            uint32_t taxon, nft_seq;
            uint16_t flags, tffee;
            uint8_t *token_id_ptr = &reg_entry_buf[HOST_TOKEN_ID_OFFSET];
            GET_NFT(data_ptr, token_id_ptr, nft_exists, issuer, uri, uri_len, taxon, flags, tffee, nft_seq);
            if (!nft_exists)
                rollback(SBUF("Evernode: Token mismatch with registration."), 1);

            // Issuer of the NFT should be the registry contract.
            BUFFER_EQUAL(nft_exists, issuer, hook_accid, ACCOUNT_ID_SIZE);
            if (!nft_exists)
                rollback(SBUF("Evernode: NFT Issuer mismatch with registration."), 1);

            // Check whether the NFT URI is starting with 'evrhost'.
            BUFFER_EQUAL_STR(nft_exists, uri, 7, EVR_HOST);
            if (!nft_exists)
                rollback(SBUF("Evernode: NFT URI is mismatch with registration."), 1);

            uint8_t emithash[HASH_SIZE];
            uint8_t index_for_burnability = 0;
            if ((flags & (1 << index_for_burnability)) != 0)
            {
                // Reserve for two transaction emissions.
                etxn_reserve(2);

                // Burn Registration NFT.
                uint8_t txn_out[PREPARE_NFT_BURN_SIZE];
                PREPARE_NFT_BURN(txn_out, token_id_ptr, data_ptr);

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
                PREPARE_PAYMENT_PRUNED_HOST_REBATE(tx_buf, amt_out_return, 0, data_ptr);
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
                PREPARE_MIN_PAYMENT_PRUNED_HOST(tx_buf, 1, data_ptr);

                if (emit(SBUF(emithash), SBUF(tx_buf)) < 0)
                    rollback(SBUF("Evernode: Minimum XRP to host account failed."), 1);
                trace(SBUF("emit hash: "), SBUF(emithash), 1);
            }

            accept(SBUF("Evernode: Dead Host Pruning successful."), 0);
        }
        else if (is_host_transfer)
        {
            // Check for registration entry, if transferee is different from transfer (transferring to a new account).
            int is_host_as_transferee = 0;
            BUFFER_EQUAL(is_host_as_transferee, data_ptr, account_field, 20);
            if (is_host_as_transferee == 0)
            {
                HOST_ADDR_KEY(data_ptr); // Generate account key for transferee.
                uint8_t reg_entry_buf[HOST_ADDR_VAL_SIZE];
                if (state(SBUF(reg_entry_buf), SBUF(STP_HOST_ADDR)) != DOESNT_EXIST)
                    rollback(SBUF("Evernode: New transferee also a registered host."), 1);
            }

            // Check whether this host has an initiated transfer.
            uint8_t host_transfer_flag = host_addr[HOST_TRANSFER_FLAG_OFFSET];
            if (host_transfer_flag == PENDING_TRANSFER)
                rollback(SBUF("Evernode: Host has a pending transfer."), 1);

            // Check whether there is an already initiated transfer for the transferee
            TRANSFEREE_ADDR_KEY(data_ptr);
            // <transferring_host_address(20)><registration_ledger(8)><token_id(20)>
            uint8_t transferee_addr[TRANSFEREE_ADDR_VAL_SIZE];

            if (state(SBUF(transferee_addr), SBUF(STP_TRANSFEREE_ADDR)) != DOESNT_EXIST)
                rollback(SBUF("Evernode: There is a previously initiated transfer for this transferee."), 1);

            // Saving the Pending transfer in Hook States.
            COPY_BUF(transferee_addr, TRANSFER_HOST_ADDRESS_OFFSET, account_field, 0, ACCOUNT_ID_SIZE);
            INT64_TO_BUF(&transferee_addr[TRANSFER_HOST_LEDGER_OFFSET], cur_ledger_seq);
            COPY_BUF(transferee_addr, TRANSFER_HOST_TOKEN_ID_OFFSET, host_addr, HOST_TOKEN_ID_OFFSET, NFT_TOKEN_ID_SIZE);

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
    }
    else if (common_params[CHAIN_IDX_PARAM_OFFSET] != 2)
    {
    }

    accept(SBUF("Evernode: Transaction is not handled in Hook Position 2."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}