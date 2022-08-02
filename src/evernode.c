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
        int is_outgoing = 0;
        BUFFER_EQUAL(is_outgoing, hook_accid, account_field, 20);
        if (is_outgoing)
            accept(SBUF("Evernode: Outgoing transaction. Passing."), 0);

        // Memos
        uint8_t memos[MAX_MEMO_SIZE];
        int64_t memos_len = otxn_field(SBUF(memos), sfMemos);

        if (!memos_len)
            accept(SBUF("Evernode: No memos found, Unhandled transaction."), 1);

        uint8_t *memo_ptr, *type_ptr, *format_ptr, *data_ptr;
        uint32_t memo_len, type_len, format_len, data_len;
        GET_MEMO(0, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len);

        if (txn_type == ttNFT_ACCEPT_OFFER)
        {
            // Host deregistration nft accept.
            int is_host_de_reg_nft_accept = 0;
            BUFFER_EQUAL_STR(is_host_de_reg_nft_accept, type_ptr, type_len, HOST_POST_DEREG);
            if (is_host_de_reg_nft_accept)
            {
                BUFFER_EQUAL_STR(is_host_de_reg_nft_accept, format_ptr, format_len, FORMAT_HEX);
                if (!is_host_de_reg_nft_accept)
                    rollback(SBUF("Evernode: Memo format should be hex."), 1);

                // Check whether the host address state is deleted.
                HOST_ADDR_KEY(account_field);
                uint8_t host_addr[HOST_ADDR_VAL_SIZE];
                if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) != DOESNT_EXIST)
                    rollback(SBUF("Evernode: The host address state exists."), 1);

                // Check whether the host token id state is deleted.
                TOKEN_ID_KEY(data_ptr);
                uint8_t token_id[TOKEN_ID_VAL_SIZE];
                if (state(SBUF(token_id), SBUF(STP_TOKEN_ID)) != DOESNT_EXIST)
                    rollback(SBUF("Evernode: The host token id state exists."), 1);

                if (reserved == STRONG_HOOK)
                {
                    if (hook_again() != 1)
                        rollback(SBUF("Evernode: Hook again faild on post deregistration."), 1);

                    accept(SBUF("Host de-registration nft accept successful."), 0);
                }
                else if (reserved == AGAIN_HOOK)
                {
                    // Check the ownership of the NFT to the hook before proceeding.
                    int nft_exists;
                    uint8_t issuer[20], uri[64], uri_len;
                    uint32_t taxon, nft_seq;
                    uint16_t flags, tffee;
                    GET_NFT(hook_accid, data_ptr, nft_exists, issuer, uri, uri_len, taxon, flags, tffee, nft_seq);
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

                    // Burn the NFT.
                    etxn_reserve(1);

                    uint8_t txn_out[PREPARE_NFT_BURN_SIZE];
                    PREPARE_NFT_BURN(txn_out, data_ptr, hook_accid);

                    uint8_t emithash[HASH_SIZE];
                    if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting NFT burn txn failed"), 1);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);

                    accept(SBUF("Host de-registration nft burn successful."), 0);
                }
            }
        }
        else if (reserved == STRONG_HOOK && txn_type == ttPAYMENT)
        {
            // Get transaction hash(id).
            uint8_t txid[HASH_SIZE];
            int32_t txid_len = otxn_id(SBUF(txid), 0);
            if (txid_len < HASH_SIZE)
                rollback(SBUF("Evernode: transaction id missing."), 1);

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

            if (is_xrp)
            {
                // Host initialization.
                int is_initialize = 0;
                BUFFER_EQUAL_STR_GUARD(is_initialize, type_ptr, type_len, INITIALIZE, 1);
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
                    int already_intialized = 0; // For Beta test purposes
                    uint8_t host_count_buf[8];
                    if (state(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) != DOESNT_EXIST)
                    {
                        already_intialized = 1;
                        // rollback(SBUF("Evernode: State is already initialized."), 1);
                    }

                    // Initialize the state.
                    if (!already_intialized)
                    {
                        const uint64_t zero = 0;
                        SET_UINT_STATE_VALUE(zero, STK_HOST_COUNT, "Evernode: Could not initialize state for host count.");
                        SET_UINT_STATE_VALUE(zero, STK_MOMENT_BASE_IDX, "Evernode: Could not initialize state for moment base index.");
                        SET_UINT_STATE_VALUE(DEF_HOST_REG_FEE, STK_HOST_REG_FEE, "Evernode: Could not initialize state for reg fee.");
                        SET_UINT_STATE_VALUE(DEF_MAX_REG, STK_MAX_REG, "Evernode: Could not initialize state for maximum registrants.");

                        const uint32_t cur_moment = cur_ledger_seq / DEF_MOMENT_SIZE;
                        // <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)>
                        uint8_t reward_info[REWARD_INFO_VAL_SIZE];
                        reward_info[EPOCH_OFFSET] = 1;
                        UINT32_TO_BUF(&reward_info[SAVED_MOMENT_OFFSET], cur_moment);
                        UINT32_TO_BUF(&reward_info[PREV_MOMENT_ACTIVE_HOST_COUNT_OFFSET], zero);
                        UINT32_TO_BUF(&reward_info[CUR_MOMENT_ACTIVE_HOST_COUNT_OFFSET], zero);
                        INT64_TO_BUF(&reward_info[EPOCH_POOL_OFFSET], float_set(0, DEF_EPOCH_REWARD_AMOUNT));
                        if (state_set(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO)) < 0)
                            rollback(SBUF("Evernode: Could not set state for reward info."), 1);

                        if (state_set(issuer_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_ISSUER_ADDR)) < 0)
                            rollback(SBUF("Evernode: Could not set state for issuer account."), 1);

                        if (state_set(foundation_ptr, ACCOUNT_ID_SIZE, SBUF(CONF_FOUNDATION_ADDR)) < 0)
                            rollback(SBUF("Evernode: Could not set state for foundation account."), 1);
                    }

                    SET_UINT_STATE_VALUE(DEF_MOMENT_SIZE, CONF_MOMENT_SIZE, "Evernode: Could not initialize state for moment size.");
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

                    accept(SBUF("Evernode: Initialization successful."), 0);
                }

                // Host deregistration.
                int is_host_de_reg = 0;
                BUFFER_EQUAL_STR(is_host_de_reg, type_ptr, type_len, HOST_DE_REG);
                if (is_host_de_reg)
                {
                    int is_format_hex = 0;
                    BUFFER_EQUAL_STR(is_format_hex, format_ptr, format_len, FORMAT_HEX);
                    if (!is_format_hex)
                        rollback(SBUF("Evernode: Format should be hex for host deregistration."), 1);

                    HOST_ADDR_KEY(account_field); // Generate host account key.
                    // Check for registration entry.
                    uint8_t reg_entry_buf[HOST_ADDR_VAL_SIZE];
                    if (state(SBUF(reg_entry_buf), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                        rollback(SBUF("Evernode: This host is not registered."), 1);

                    int is_token_match = 0;
                    BUFFER_EQUAL(is_token_match, data_ptr, reg_entry_buf + HOST_TOKEN_ID_OFFSET, NFT_TOKEN_ID_SIZE);

                    if (!is_token_match)
                        rollback(SBUF("Evernode: Token id sent doesn't match with the registered NFT."), 1);

                    TOKEN_ID_KEY((uint8_t *)(reg_entry_buf + HOST_TOKEN_ID_OFFSET)); // Generate token id key.

                    // Check the ownership of the NFT to this user before proceeding.
                    int nft_exists;
                    uint8_t issuer[20], uri[64], uri_len;
                    uint32_t taxon, nft_seq;
                    uint16_t flags, tffee;
                    uint8_t *token_id_ptr = &reg_entry_buf[HOST_TOKEN_ID_OFFSET];
                    GET_NFT(account_field, token_id_ptr, nft_exists, issuer, uri, uri_len, taxon, flags, tffee, nft_seq);
                    if (!nft_exists)
                        rollback(SBUF("Evernode: Token mismatch with registration."), 1);

                    // Issuer of the NFT should be the registry contract.
                    BUFFER_EQUAL(nft_exists, issuer, hook_accid, ACCOUNT_ID_SIZE);
                    if (!nft_exists)
                        rollback(SBUF("Evernode: NFT Issuer mismatch with registration."), 1);

                    // Delete registration entries.
                    if (state_set(0, 0, SBUF(STP_TOKEN_ID)) < 0 || state_set(0, 0, SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not delete host registration entry."), 1);

                    // Reduce host count by 1.
                    uint64_t host_count;
                    GET_HOST_COUNT(host_count);
                    host_count -= 1;
                    SET_HOST_COUNT(host_count);

                    uint64_t reg_fee = UINT64_FROM_BUF(reg_entry_buf + HOST_REG_FEE_OFFSET);
                    uint64_t fixed_reg_fee;
                    GET_CONF_VALUE(fixed_reg_fee, CONF_FIXED_REG_FEE, "Evernode: Could not get fixed reg fee.");

                    uint8_t issuer_accid[20];
                    uint8_t foundation_accid[20];
                    if (state(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR)) < 0 || state(SBUF(foundation_accid), SBUF(CONF_FOUNDATION_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not get issuer or foundation account id."), 1);

                    int64_t amount_half = reg_fee > fixed_reg_fee ? reg_fee / 2 : 0;

                    if (reg_fee > fixed_reg_fee)
                    {
                        // Take the moment base idx from config.
                        uint64_t moment_base_idx;
                        GET_CONF_VALUE(moment_base_idx, STK_MOMENT_BASE_IDX, "Evernode: Could not get moment base index.");
                        TRACEVAR(moment_base_idx);

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
                    uint8_t buy_tx_buf[PREPARE_NFT_BUY_OFFER_SIZE];
                    PREPARE_NFT_BUY_OFFER(buy_tx_buf, amt_out, account_field, (uint8_t *)(reg_entry_buf + HOST_TOKEN_ID_OFFSET));
                    uint8_t emithash[HASH_SIZE];
                    if (emit(SBUF(emithash), SBUF(buy_tx_buf)) < 0)
                        rollback(SBUF("Evernode: Emitting buying offer to NFT failed."), 1);

                    accept(SBUF("Evernode: Host de-registration successful."), 0);
                }

                // Host heartbeat.
                int is_host_heartbeat = 0;
                BUFFER_EQUAL_STR(is_host_heartbeat, type_ptr, type_len, HEARTBEAT);
                if (is_host_heartbeat)
                {
                    HOST_ADDR_KEY(account_field);
                    // <token_id(32)><country_code(2)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
                    // <no_of_total_instances(4)><no_of_active_instances(4)><last_heartbeat_ledger(8)><version(3)>
                    uint8_t host_addr[HOST_ADDR_VAL_SIZE];
                    if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not get host address state."), 1);

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

                    const uint8_t *heartbeat_ptr = &host_addr[HOST_HEARTBEAT_LEDGER_IDX_OFFSET];
                    const int64_t last_heartbeat_idx = INT64_FROM_BUF(heartbeat_ptr);

                    INT64_TO_BUF(heartbeat_ptr, cur_ledger_seq);

                    if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not set state for heartbeat."), 1);

                    // Take the moment base idx from config.
                    uint64_t moment_base_idx;
                    GET_CONF_VALUE(moment_base_idx, STK_MOMENT_BASE_IDX, "Evernode: Could not get moment base index.");
                    TRACEVAR(moment_base_idx);

                    // Take the moment size from config.
                    uint16_t moment_size;
                    GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");
                    TRACEVAR(moment_size);

                    // Take the heartbeat freaquency.
                    uint16_t heartbeat_freq;
                    GET_CONF_VALUE(heartbeat_freq, CONF_HOST_HEARTBEAT_FREQ, "Evernode: Could not get heartbeat frequency.");
                    TRACEVAR(heartbeat_freq);

                    const uint32_t last_heartbeat_moment = last_heartbeat_idx != 0 ? (last_heartbeat_idx - moment_base_idx) / moment_size : 0;
                    const uint32_t cur_moment = (cur_ledger_seq - moment_base_idx) / moment_size;

                    // Skip if already sent a heartbeat in this moment.
                    if (cur_moment > last_heartbeat_moment)
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
                        // the eward quota is not 0.
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

                int is_host_update_reg = 0;
                BUFFER_EQUAL_STR_GUARD(is_host_update_reg, type_ptr, type_len, HOST_UPDATE_REG, 1);
                if (is_host_update_reg)
                {
                    int is_format_plain_text = 0;
                    BUFFER_EQUAL_STR_GUARD(is_format_plain_text, format_ptr, format_len, FORMAT_TEXT, 1);
                    if (!is_format_plain_text)
                        rollback(SBUF("Evernode: Instance update info format not supported."), 1);

                    HOST_ADDR_KEY(account_field);
                    // <token_id(32)><country_code(2)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
                    // <no_of_total_instances(4)><no_of_active_instances(4)><last_heartbeat_ledger(8)><version(3)>
                    uint8_t host_addr[HOST_ADDR_VAL_SIZE];
                    if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not get host address state."), 1);

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

                // Dead Host Prune.
                int is_dead_host_prune = 0;
                BUFFER_EQUAL_STR(is_dead_host_prune, type_ptr, type_len, DEAD_HOST_PRUNE);
                if (is_dead_host_prune)
                {
                    int is_format_text = 0;
                    BUFFER_EQUAL_STR(is_format_text, format_ptr, format_len, FORMAT_HEX);
                    if (!is_format_text)
                        rollback(SBUF("Evernode: Format should be text to prune the host."), 1);

                    HOST_ADDR_KEY(data_ptr); // Generate host account key.
                    // Check for registration entry.
                    uint8_t reg_entry_buf[HOST_ADDR_VAL_SIZE];
                    if (state(SBUF(reg_entry_buf), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                        rollback(SBUF("Evernode: This host is not registered."), 1);

                    // Take the moment size from config.
                    uint16_t moment_size;
                    GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");
                    TRACEVAR(moment_size);

                    uint8_t *host_hearbeat_ledger_ptr = &reg_entry_buf[HOST_HEARTBEAT_LEDGER_IDX_OFFSET];
                    int64_t last_hearbeat_ledger = INT64_FROM_BUF(host_hearbeat_ledger_ptr);
                    TRACEVAR(cur_ledger_seq);
                    TRACEVAR(last_hearbeat_ledger);
                    int64_t heartbeat_delay = (cur_ledger_seq - last_hearbeat_ledger) / moment_size;
                    TRACEVAR(heartbeat_delay);

                    // Take the maximun tolerable downtime from config.
                    uint16_t max_tolerable_downtime;
                    GET_CONF_VALUE(max_tolerable_downtime, CONF_MAX_TOLERABLE_DOWNTIME, "Evernode: Could not get the maximum tolerable downtime from the state.");
                    TRACEVAR(max_tolerable_downtime);

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

                    uint64_t reg_fee = UINT64_FROM_BUF(reg_entry_buf + HOST_REG_FEE_OFFSET);
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
                        int64_t amount_half = reg_fee / 2;

                        SET_AMOUNT_OUT(amt_out_return, EVR_TOKEN, issuer_accid, float_set(0, amount_half));

                        // Prepare transaction to send 50% of reg fee to host account.
                        PREPARE_PAYMENT_PRUNED_HOST_REBATE(tx_buf, amt_out_return, 0, data_ptr);
                        if (emit(SBUF(emithash), SBUF(tx_buf)) < 0)
                            rollback(SBUF("Evernode: Rebating 1/2 reg fee to host account failed."), 1);
                        trace(SBUF("emit hash: "), SBUF(emithash), 1);

                        // BEGIN: Update the epoch Reward pool.

                        // Take the moment base idx from config.
                        uint64_t moment_base_idx;
                        GET_CONF_VALUE(moment_base_idx, STK_MOMENT_BASE_IDX, "Evernode: Could not get moment base index.");
                        TRACEVAR(moment_base_idx);

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
                    else
                    {
                        uint8_t tx_buf[PREPARE_MIN_PAYMENT_PRUNED_HOST_SIZE];
                        // Prepare MIN XRP transaction to host about pruning.
                        PREPARE_MIN_PAYMENT_PRUNED_HOST(tx_buf, 1, data_ptr);

                        if (emit(SBUF(emithash), SBUF(tx_buf)) < 0)
                            rollback(SBUF("Evernode: Minimum XRP to host account failed."), 1);
                        trace(SBUF("emit hash: "), SBUF(emithash), 1);
                    }
                }

                accept(SBUF("Evernode: Dead Host Pruning successful."), 0);
            }
            else
            {
                // IOU payment.
                int64_t amt = slot_float(amt_slot);
                if (amt < 0)
                    rollback(SBUF("Evernode: Could not parse amount."), 1);

                int64_t amt_drops = float_int(amt, 6, 0);
                if (amt_drops < 0)
                    rollback(SBUF("Evernode: Could not parse amount."), 1);
                int64_t amt_int = amt_drops / 1000000;

                uint8_t amount_buffer[AMOUNT_BUF_SIZE];
                int64_t result = slot(SBUF(amount_buffer), amt_slot);
                if (result != AMOUNT_BUF_SIZE)
                    rollback(SBUF("Evernode: Could not dump sfAmount"), 1);

                uint8_t issuer_accid[20];
                if (state(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR)) < 0)
                    rollback(SBUF("Evernode: Could not get issuer address state."), 1);

                int is_evr;
                IS_EVR(is_evr, amount_buffer, issuer_accid);

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
                        rollback(SBUF("Evernode: Memo format should be text."), 1);

                    // Take the host reg fee from config.
                    int64_t host_reg_fee;
                    GET_CONF_VALUE(host_reg_fee, STK_HOST_REG_FEE, "Evernode: Could not get host reg fee state.");
                    TRACEVAR(host_reg_fee);

                    if (float_compare(amt, float_set(0, host_reg_fee), COMPARE_LESS) == 1)
                        rollback(SBUF("Evernode: Amount sent is less than the minimum fee for host registration."), 1);

                    // Checking whether this host is already registered.
                    HOST_ADDR_KEY(account_field);
                    // <token_id(32)><country_code(2)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
                    // <no_of_total_instances(4)><no_of_active_instances(4)><last_heartbeat_ledger(8)><version(3)>
                    uint8_t host_addr[HOST_ADDR_VAL_SIZE];

                    // <host_address(20)><cpu_model_name(40)><cpu_count(2)><cpu_speed(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)>
                    uint8_t token_id[TOKEN_ID_VAL_SIZE];

                    if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) != DOESNT_EXIST)
                        rollback(SBUF("Evernode: Host already registered."), 1);

                    // Generate the NFT token id.

                    // Take the account token squence from keylet.
                    uint8_t keylet[34];
                    if (util_keylet(SBUF(keylet), KEYLET_ACCOUNT, SBUF(hook_accid), 0, 0, 0, 0) != 34)
                        rollback(SBUF("Evernode: Could not generate the keylet for KEYLET_ACCOUNT."), 10);

                    int64_t slot_no = slot_set(SBUF(keylet), 0);
                    if (slot_no < 0)
                        rollback(SBUF("Evernode: Could not set keylet in slot"), 10);

                    int64_t token_seq_slot = slot_subfield(slot_no, sfMintedNFTokens, 0);
                    uint32_t token_seq = 0;
                    if (token_seq_slot >= 0)
                    {
                        uint8_t token_seq_buf[4];
                        token_seq_slot = slot(SBUF(token_seq_buf), token_seq_slot);
                        token_seq = UINT32_FROM_BUF(token_seq_buf);
                    }
                    else if (token_seq_slot != DOESNT_EXIST)
                        rollback(SBUF("Evernode: Could not find sfMintedTokens on hook account"), 20);
                    TRACEVAR(token_seq);

                    // If there are multiple flags, we can perform "Bitwise OR" to apply them all to tflag.
                    uint16_t tflag = tfBurnable;
                    uint32_t taxon = 0;
                    uint16_t tffee = 0;

                    uint8_t nft_token_id[NFT_TOKEN_ID_SIZE];
                    GENERATE_NFT_TOKEN_ID(nft_token_id, tflag, tffee, hook_accid, taxon, token_seq);
                    trace("NFT token id:", 13, SBUF(nft_token_id), 1);

                    TOKEN_ID_KEY(nft_token_id);

                    CLEARBUF(host_addr);
                    COPY_BUF(host_addr, 0, nft_token_id, 0, NFT_TOKEN_ID_SIZE);
                    COPY_BUF(host_addr, HOST_COUNTRY_CODE_OFFSET, data_ptr, 0, COUNTRY_CODE_LEN);

                    // Read instace details from the memo.
                    // We cannot predict the lengths of the numarical values.
                    // So we scan bytes and keep pointers and lengths to set in host address buffer.
                    uint32_t section_number = 0;
                    uint8_t *cpu_microsec_ptr, *ram_mb_ptr, *disk_mb_ptr, *total_ins_count_ptr, *cpu_model_ptr, *cpu_count_ptr, *cpu_speed_ptr, *description_ptr;
                    uint32_t cpu_microsec_len = 0, ram_mb_len = 0, disk_mb_len = 0, total_ins_count_len = 0, cpu_model_len = 0, cpu_count_len = 0, cpu_speed_len = 0, description_len = 0;

                    for (int i = 2; GUARD(MAX_MEMO_DATA_LEN), i < data_len; ++i)
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
                            if (total_ins_count_len == 0)
                                total_ins_count_ptr = str_ptr;
                            total_ins_count_len++;
                        }
                        else if (section_number == 5)
                        {
                            if (cpu_model_len == 0)
                                cpu_model_ptr = str_ptr;
                            cpu_model_len++;
                        }
                        else if (section_number == 6)
                        {
                            if (cpu_count_len == 0)
                                cpu_count_ptr = str_ptr;
                            cpu_count_len++;
                        }
                        else if (section_number == 7)
                        {
                            if (cpu_speed_len == 0)
                                cpu_speed_ptr = str_ptr;
                            cpu_speed_len++;
                        }
                        else if (section_number == 8)
                        {
                            if (description_len == 0)
                                description_ptr = str_ptr;
                            description_len++;
                        }
                    }

                    // Take the unsigned int values.
                    uint16_t cpu_count, cpu_speed;
                    uint32_t cpu_microsec, ram_mb, disk_mb, total_ins_count;
                    STR_TO_UINT(cpu_microsec, cpu_microsec_ptr, cpu_microsec_len);
                    STR_TO_UINT(ram_mb, ram_mb_ptr, ram_mb_len);
                    STR_TO_UINT(disk_mb, disk_mb_ptr, disk_mb_len);
                    STR_TO_UINT(total_ins_count, total_ins_count_ptr, total_ins_count_len);
                    STR_TO_UINT(cpu_count, cpu_count_ptr, cpu_count_len);
                    STR_TO_UINT(cpu_speed, cpu_speed_ptr, cpu_speed_len);

                    // Populate tvalues to the state address buffer and set state.
                    CLEAR_BUF(host_addr, HOST_RESERVED_OFFSET, 8);
                    COPY_BUF_NON_CONST_LEN_GUARDM(host_addr, HOST_DESCRIPTION_OFFSET, description_ptr, 0, description_len, DESCRIPTION_LEN, 1, 1);
                    if (description_len < DESCRIPTION_LEN)
                        CLEAR_BUF_NON_CONST_LEN(host_addr, HOST_DESCRIPTION_OFFSET + description_len, DESCRIPTION_LEN - description_len, DESCRIPTION_LEN);
                    INT64_TO_BUF(&host_addr[HOST_REG_LEDGER_OFFSET], cur_ledger_seq);
                    UINT64_TO_BUF(&host_addr[HOST_REG_FEE_OFFSET], host_reg_fee);
                    UINT32_TO_BUF(&host_addr[HOST_TOT_INS_COUNT_OFFSET], total_ins_count);

                    if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not set state for host_addr."), 1);

                    // Populate the values to the token id buffer and set state.
                    COPY_BUF(token_id, HOST_ADDRESS_OFFSET, account_field, 0, ACCOUNT_ID_SIZE);
                    COPY_BUF_NON_CONST_LEN_GUARDM(token_id, HOST_CPU_MODEL_NAME_OFFSET, cpu_model_ptr, 0, cpu_model_len, CPU_MODEl_NAME_LEN, 1, 1);
                    if (cpu_model_len < CPU_MODEl_NAME_LEN)
                        CLEAR_BUF_NON_CONST_LEN(token_id, HOST_CPU_MODEL_NAME_OFFSET + cpu_model_len, CPU_MODEl_NAME_LEN - cpu_model_len, CPU_MODEl_NAME_LEN);
                    UINT16_TO_BUF(&token_id[HOST_CPU_COUNT_OFFSET], cpu_count);
                    UINT16_TO_BUF(&token_id[HOST_CPU_SPEED_OFFSET], cpu_speed);
                    UINT32_TO_BUF(&token_id[HOST_CPU_MICROSEC_OFFSET], cpu_microsec);
                    UINT32_TO_BUF(&token_id[HOST_RAM_MB_OFFSET], ram_mb);
                    UINT32_TO_BUF(&token_id[HOST_DISK_MB_OFFSET], disk_mb);

                    if (state_set(SBUF(token_id), SBUF(STP_TOKEN_ID)) < 0)
                        rollback(SBUF("Evernode: Could not set state for token_id."), 1);

                    uint32_t host_count;
                    GET_HOST_COUNT(host_count);
                    host_count += 1;
                    SET_HOST_COUNT(host_count);

                    // Take the fixed reg fee from config.
                    int64_t conf_fixed_reg_fee;
                    GET_CONF_VALUE(conf_fixed_reg_fee, CONF_FIXED_REG_FEE, "Evernode: Could not get fixed reg fee state.");
                    TRACEVAR(conf_fixed_reg_fee);

                    // Take the fixed theoritical maximum registrants value from config.
                    uint64_t conf_max_reg;
                    GET_CONF_VALUE(conf_max_reg, STK_MAX_REG, "Evernode: Could not get max reg fee state.");
                    TRACEVAR(conf_max_reg);

                    // int max_reached = 0;
                    // if (host_reg_fee > conf_fixed_reg_fee && host_count > (conf_max_reg / 2))
                    // {
                    //     max_reached = 1;
                    //     etxn_reserve(host_count + 3);
                    // }
                    // else
                    etxn_reserve(3);

                    // Froward 5 EVRs to foundation.
                    uint8_t foundation_accid[20];
                    if (state(SBUF(foundation_accid), SBUF(CONF_FOUNDATION_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not get foundation account address state."), 1);

                    uint8_t amt_out[AMOUNT_BUF_SIZE];
                    SET_AMOUNT_OUT(amt_out, EVR_TOKEN, issuer_accid, float_set(0, conf_fixed_reg_fee));

                    // Create the outgoing hosting token txn.
                    uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                    PREPARE_PAYMENT_SIMPLE_TRUSTLINE(txn_out, amt_out, foundation_accid, 0, 0);

                    uint8_t emithash[32];
                    if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting EVR forward txn failed"), 1);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);

                    // Mint the nft token.
                    // Transaction URI would be the 'evrhost' + registration transaction hash.
                    uint8_t uri[39];
                    COPY_BUF_GUARDM(uri, 0, EVR_HOST, 0, 7, 1, 1);
                    COPY_BUF_GUARDM(uri, 7, txid, 0, HASH_SIZE, 1, 2);

                    uint8_t nft_txn_out[PREPARE_NFT_MINT_SIZE(sizeof(uri))];
                    PREPARE_NFT_MINT(nft_txn_out, tflag, tffee, taxon, uri, sizeof(uri));

                    if (emit(SBUF(emithash), SBUF(nft_txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting NFT mint txn failed"), 1);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);

                    // Amount will be 0.
                    uint8_t offer_txn_out[PREPARE_NFT_SELL_OFFER_SIZE];
                    PREPARE_NFT_SELL_OFFER(offer_txn_out, 0, account_field, nft_token_id);

                    if (emit(SBUF(emithash), SBUF(offer_txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting offer txn failed"), 1);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);

                    // We disable the registration fee halving code until the logic is refined.
                    // if (max_reached)
                    // {
                    //     uint8_t state_buf[8] = {0};

                    //     host_reg_fee /= 2;
                    //     UINT64_TO_BUF(state_buf, host_reg_fee);
                    //     if (state_set(SBUF(state_buf), SBUF(STK_HOST_REG_FEE)) < 0)
                    //         rollback(SBUF("Evernode: Could not update the state for host reg fee."), 1);

                    //     conf_max_reg *= 2;
                    //     UINT64_TO_BUF(state_buf, conf_max_reg);
                    //     if (state_set(SBUF(state_buf), SBUF(STK_MAX_REG)) < 0)
                    //         rollback(SBUF("Evernode: Could not update state for max theoritical registrants."), 1);

                    //     // Refund the EVR balance.
                    //     for (int i = 0; GUARD(DEF_MAX_REG), i < token_seq + 1; ++i)
                    //     {
                    //         // Loop through all the possible token sequences and generate the token ids.
                    //         uint8_t lookup_id[NFT_TOKEN_ID_SIZE];
                    //         GENERATE_NFT_TOKEN_ID_GUARD(lookup_id, tflag, tffee, hook_accid, taxon, i, DEF_MAX_REG);

                    //         // If the token id exists in the state (host is still registered),
                    //         // Rebate the halved registration fee.
                    //         TOKEN_ID_KEY_GUARD(lookup_id, DEF_MAX_REG);
                    //         uint8_t token_id[TOKEN_ID_VAL_SIZE];
                    //         if (state(SBUF(token_id), SBUF(STP_TOKEN_ID)) != DOESNT_EXIST)
                    //         {
                    //             uint8_t *host_accid = &token_id;
                    //             uint8_t amt_out[AMOUNT_BUF_SIZE];
                    //             SET_AMOUNT_OUT_GUARD(amt_out, EVR_TOKEN, issuer_accid, float_set(0, host_reg_fee), DEF_MAX_REG);

                    //             // Create the outgoing hosting token txn.
                    //             uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                    //             PREPARE_PAYMENT_SIMPLE_TRUSTLINE_GUARDM(txn_out, amt_out, host_accid, 0, 0, DEF_MAX_REG, 1);

                    //             if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    //                 rollback(SBUF("Evernode: Emitting txn failed"), 1);
                    //             trace(SBUF("emit hash: "), SBUF(emithash), 1);

                    //             // Updating the current reg fee in the host state.
                    //             HOST_ADDR_KEY_GUARD(host_accid, DEF_MAX_REG);
                    //             // <token_id(32)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
                    //             // <no_of_total_instances(4)><no_of_active_instances(4)><last_heartbeat_ledger(8)>
                    //             uint8_t rebate_host_addr[HOST_ADDR_VAL_SIZE];
                    //             if (state(SBUF(rebate_host_addr), SBUF(STP_HOST_ADDR)) < 0)
                    //                 rollback(SBUF("Evernode: Could not get host address state."), 1);
                    //             UINT64_TO_BUF(&rebate_host_addr[HOST_REG_FEE_OFFSET], host_reg_fee);
                    //             if (state_set(SBUF(rebate_host_addr), SBUF(STP_HOST_ADDR)) < 0)
                    //                 rollback(SBUF("Evernode: Could not update host address state."), 1);
                    //         }
                    //     }
                    // }

                    accept(SBUF("Host registration successful."), 0);
                }
            }
        }
        else if (txn_type == ttPAYMENT)
        {
        }
    }

    accept(SBUF("Evernode: Transaction is not handled."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
