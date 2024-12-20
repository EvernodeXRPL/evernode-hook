#include "registry.h"

/**
 * Only the incoming transactions are handled. Outgoing transactions are accepted directly.
 * Manages evernode host registry related operations.
 * Handles Host registration, De registration, Update registration, Host pruning, Dud host removal, Transfer registration, Registration fee management,
 * Rebates and Applying self hook updates.
 * Does not have states. Accesses governance states in as foreign.
 * Supported transaction types: Payment(IOU|XRP), URITokenCreateSellOffer(XRP)
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
    TRACESTR(VERSION);

    if (!ACTIVE(ACTIVATION_UNIXTIME))
        rollback(SBUF("Not active yet."), __LINE__);

    CHECK_PARTIAL_PAYMENT();

    // Getting the hook account id.
    unsigned char hook_accid[ACCOUNT_ID_SIZE];
    hook_account((uint32_t)hook_accid, ACCOUNT_ID_SIZE);

    // Next fetch the sfAccount field from the originating transaction
    uint8_t account_field[ACCOUNT_ID_SIZE];
    int32_t account_field_len = otxn_field(SBUF(account_field), sfAccount);

    // ASSERT_FAILURE_MSG >> sfAccount field is missing.
    ASSERT(account_field_len == ACCOUNT_ID_SIZE);

    /**
     * Accept
     * - any transaction that is not a payment or a URITokenCreateSellOffer.
     * - any outgoing transactions without further processing.
     */
    int64_t txn_type = otxn_type();
    if ((!(txn_type == ttPAYMENT || txn_type == ttURI_TOKEN_CREATE_SELL_OFFER)) || BUFFER_EQUAL_20(hook_accid, account_field))
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

    uint8_t state_hook_accid[ACCOUNT_ID_SIZE] = {0};

    // ASSERT_FAILURE_MSG >> Error getting the state hook address from params.
    ASSERT(hook_param(SBUF(state_hook_accid), SBUF(PARAM_STATE_HOOK_KEY)) >= 0);

    uint8_t txid[HASH_SIZE];
    const int32_t txid_len = otxn_id(SBUF(txid), 0);

    // ASSERT_FAILURE_MSG >> U transaction id missing.
    ASSERT(txid_len == HASH_SIZE);

    // <transition index><transition_moment><index_type>
    uint8_t moment_base_info[MOMENT_BASE_INFO_VAL_SIZE] = {0};

    // ASSERT_FAILURE_MSG >> Could not get moment base info state.
    ASSERT(state_foreign(SBUF(moment_base_info), SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF) >= 0);

    uint64_t moment_base_idx = UINT64_FROM_BUF_LE(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);
    uint32_t prev_transition_moment = UINT32_FROM_BUF_LE(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]);
    // If state does not exist, take the moment type from default constant.
    uint8_t cur_moment_type = moment_base_info[MOMENT_TYPE_OFFSET];
    uint64_t cur_idx = cur_moment_type == TIMESTAMP_MOMENT_TYPE ? cur_ledger_timestamp : cur_ledger_seq;

    // <fee_base_avg(uint32_t)><avg_changed_idx(uint64_t)><avg_accumulator(uint32_t)><counter(uint16_t)>
    uint8_t trx_fee_base_info[TRX_FEE_BASE_INFO_VAL_SIZE] = {0};
    // ASSERT_FAILURE_MSG >> Could not get transaction fee base info state.
    ASSERT(state_foreign(SBUF(trx_fee_base_info), SBUF(STK_TRX_FEE_BASE_INFO), FOREIGN_REF) >= 0);
    const int64_t cur_fee_base = fee_base();
    const uint32_t fee_avg = UINT32_FROM_BUF_LE(&trx_fee_base_info[FEE_BASE_AVG_OFFSET]);

    // <busyness_detect_period(uint32_t)><busyness_detect_average(uint16_t)>
    uint8_t network_configuration[NETWORK_CONFIGURATION_VAL_SIZE] = {0};
    // ASSERT_FAILURE_MSG >> Error getting network configuration state.
    ASSERT(state_foreign(SBUF(network_configuration), SBUF(CONF_NETWORK_CONFIGURATION), FOREIGN_REF) >= 0);
    const uint16_t network_busyness_detect_average = UINT16_FROM_BUF_LE(&network_configuration[NETWORK_BUSYNESS_DETECT_AVERAGE_OFFSET]);

    // Take the moment size from config.
    uint16_t moment_size = 0;
    GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");

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

    if (txn_type == ttPAYMENT)
    {
        // ASSERT_FAILURE_MSG >> Could not slot otxn.sfAmount
        ASSERT(amt_slot >= 0);

        int64_t is_xrp = slot_type(amt_slot, 1);

        // ASSERT_FAILURE_MSG >> Could not determine sent amount type
        ASSERT(is_xrp >= 0);

        if (is_xrp)
        {
            if (EQUAL_HOST_DEREG(event_type, event_type_len))
                op_type = OP_HOST_DEREG;
            else if (EQUAL_HOST_UPDATE_REG(event_type, event_type_len))
                op_type = OP_HOST_UPDATE_REG;
            // Dead Host Prune.
            // else if (EQUAL_DEAD_HOST_PRUNE(event_type, event_type_len))
            //     op_type = OP_DEAD_HOST_PRUNE;
            // Host rebate.
            else if (EQUAL_HOST_REBATE(event_type, event_type_len))
                op_type = OP_HOST_REBATE;
            // Hook update trigger.
            else if (EQUAL_HOOK_UPDATE(event_type, event_type_len))
                op_type = OP_HOOK_UPDATE;
            // Dud host remove.
            else if (EQUAL_DUD_HOST_REMOVE(event_type, event_type_len))
                op_type = OP_DUD_HOST_REMOVE;
            // Host reputation update.
            else if (EQUAL_HOST_UPDATE_REPUTATION(event_type, event_type_len))
                op_type = OP_HOST_UPDATE_REPUTATION;
            // Foundation fund request.
            else if (EQUAL_FOUNDATION_FUND_REQ(event_type, event_type_len))
                op_type = OP_FOUNDATION_FUND_REQ;
        }
        else // IOU payment.
        {
            // Host registration.
            if (EQUAL_HOST_REG(event_type, event_type_len))
                op_type = OP_HOST_REG;
        }
    }
    else if (txn_type == ttURI_TOKEN_CREATE_SELL_OFFER)
    {
        // Host transfer.
        if (EQUAL_HOST_TRANSFER(event_type, event_type_len))
            op_type = OP_HOST_TRANSFER;
    }

    if (op_type == OP_NONE)
    {
        // PERMIT_MSG >> Transaction is not handled.
        PERMIT();
    }

    // Rebate request without vote does not have data.
    uint8_t event_data[MAX_EVENT_DATA_SIZE];
    const int64_t event_data_len = otxn_param(SBUF(event_data), SBUF(PARAM_EVENT_DATA_KEY));

    // ASSERT_FAILURE_MSG >> Error getting the event data param.
    ASSERT(!(op_type != OP_HOST_REBATE && op_type != OP_FOUNDATION_FUND_REQ && event_data_len <= 0));

    // <token_id(32)><country_code(2)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)><no_of_total_instances(4)><no_of_active_instances(4)>
    // <last_heartbeat_index(8)><version(3)><registration_timestamp(8)><transfer_flag(1)><last_vote_candidate_idx(4)><last_vote_timestamp(8)><support_vote_sent(1)><host_reputation(1)><flags(1)><transfer_timestamp(8)>
    uint8_t host_addr[HOST_ADDR_VAL_SIZE];
    // <host_address(20)><cpu_model_name(40)><cpu_count(2)><cpu_speed(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><email(40)><accumulated_reward_amount(8)>
    uint8_t token_id[TOKEN_ID_VAL_SIZE];

    // Common logic for host deregistration, heartbeat, update registration, rebate process and transfer.
    if (op_type == OP_HOST_UPDATE_REG || op_type == OP_HOST_REBATE || op_type == OP_HOST_TRANSFER ||
        op_type == OP_DEAD_HOST_PRUNE || op_type == OP_DUD_HOST_REMOVE || op_type == OP_HOST_DEREG ||
        op_type == OP_HOST_UPDATE_REPUTATION)
    {
        // Check whether dereg is sent from reputation.
        if (op_type == OP_HOST_DEREG && event_data_len == HOST_DEREG_FROM_REP_PARAM_SIZE)
            op_type = OP_HOST_DEREG_FROM_REPUTATION;

        // Generate host account key.
        if (op_type != OP_DEAD_HOST_PRUNE && op_type != OP_DUD_HOST_REMOVE && op_type != OP_HOST_UPDATE_REPUTATION && op_type != OP_HOST_DEREG_FROM_REPUTATION)
        {
            HOST_ADDR_KEY(account_field);
        }
        else
        {
            HOST_ADDR_KEY(event_data);
        }
        // Check for registration entry.

        // ASSERT_FAILURE_MSG >> This host is not registered.
        ASSERT(state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) != DOESNT_EXIST);

        TOKEN_ID_KEY((uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET)); // Generate token id key.

        // Check for token id entry.
        // ASSERT_FAILURE_MSG >> This host is not registered.
        ASSERT(state_foreign(SBUF(token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) != DOESNT_EXIST);

        // If host is a transferer it does not own a token.
        // Check whether this host has an initiated transfer.
        const int is_force_remove = (op_type == OP_DEAD_HOST_PRUNE || op_type == OP_DUD_HOST_REMOVE);
        if (!is_force_remove || host_addr[HOST_TRANSFER_FLAG_OFFSET] != PENDING_TRANSFER)
        {
            // Controlled transactions are sent from other account than host account.
            int is_controlled = (is_force_remove || op_type == OP_HOST_UPDATE_REPUTATION || op_type == OP_HOST_DEREG_FROM_REPUTATION);
            // Check the ownership of the token to this user before proceeding.
            int token_exists;
            IS_REG_TOKEN_EXIST((is_controlled ? event_data : account_field), (host_addr + HOST_TOKEN_ID_OFFSET), token_exists);

            // ASSERT_FAILURE_MSG >> Registration URIToken does not exist.
            ASSERT(token_exists);
        }
    }

    // <epoch_count(uint8_t)><first_epoch_reward_quota(uint32_t)><epoch_reward_amount(uint32_t)><reward_start_moment(uint32_t)><accumulated_reward_frequency(uint16_t)><host_reputation_threshold(uint8_t)><host_min_instance_count(uint32_t)>
    uint8_t reward_configuration[REWARD_CONFIGURATION_VAL_SIZE];
    // <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)><host_max_lease_amount(int64_t,xfl)>
    uint8_t reward_info[REWARD_INFO_VAL_SIZE];
    // ASSERT_FAILURE_MSG >> Could not get reward configuration or reward info states.
    ASSERT(!(state_foreign(SBUF(reward_configuration), SBUF(CONF_REWARD_CONFIGURATION), FOREIGN_REF) < 0 ||
             state_foreign(SBUF(reward_info), SBUF(STK_REWARD_INFO), FOREIGN_REF) < 0));

    const uint8_t epoch_count = reward_configuration[EPOCH_COUNT_OFFSET];
    const uint32_t first_epoch_reward_quota = UINT32_FROM_BUF_LE(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET]);
    const uint32_t epoch_reward_amount = UINT32_FROM_BUF_LE(&reward_configuration[EPOCH_REWARD_AMOUNT_OFFSET]);

    const uint8_t epoch = reward_info[EPOCH_OFFSET];
    const uint8_t *pool_ptr = &reward_info[EPOCH_POOL_OFFSET];
    const int64_t reward_pool_amount = INT64_FROM_BUF_LE(pool_ptr);

    uint8_t issuer_accid[ACCOUNT_ID_SIZE] = {0};
    uint8_t foundation_accid[ACCOUNT_ID_SIZE] = {0};
    uint8_t heartbeat_hook_accid[ACCOUNT_ID_SIZE] = {0};
    uint8_t reputation_hook_accid[ACCOUNT_ID_SIZE] = {0};

    // ASSERT_FAILURE_MSG >> Could not get issuer, foundation or heartbeat account id.
    ASSERT(!(state_foreign(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR), FOREIGN_REF) < 0 ||
             state_foreign(SBUF(foundation_accid), SBUF(CONF_FOUNDATION_ADDR), FOREIGN_REF) < 0 ||
             state_foreign(SBUF(heartbeat_hook_accid), SBUF(CONF_HEARTBEAT_ADDR), FOREIGN_REF) < 0 ||
             state_foreign(SBUF(reputation_hook_accid), SBUF(CONF_REPUTATION_ADDR), FOREIGN_REF) < 0));

    // Take the fixed reg fee from config.
    int64_t conf_fixed_reg_fee;
    GET_CONF_VALUE(conf_fixed_reg_fee, CONF_FIXED_REG_FEE, "Evernode: Could not get fixed reg fee state.");

    uint8_t candidate_remove_data[33];

    if (op_type == OP_FOUNDATION_FUND_REQ)
    {
        // We accept only the foundation fund request from heartbeat account.
        if (!BUFFER_EQUAL_20(heartbeat_hook_accid, account_field))
            rollback(SBUF("Evernode: Only heartbeat is allowed to send foundation fund request."), 1);

        etxn_reserve(1);

        uint8_t emithash[32];
        // Froward 5 EVRs to foundation.
        // Create the outgoing hosting token txn.
        PREPARE_PAYMENT_TRUSTLINE_TX(EVR_TOKEN, issuer_accid, float_set(0, conf_fixed_reg_fee), foundation_accid);

        // ASSERT_FAILURE_MSG >> Emitting EVR forward txn failed
        ASSERT(emit(SBUF(emithash), SBUF(PAYMENT_TRUSTLINE)) >= 0);

        // PERMIT_MSG >> Foundation fund successful.
        PERMIT();
    }
    else if (op_type == OP_HOST_REG)
    {
        // Get transaction hash(id).
        uint8_t txid[HASH_SIZE];
        const int32_t txid_len = otxn_id(SBUF(txid), 0);

        // ASSERT_FAILURE_MSG >> transaction id missing.
        ASSERT(txid_len == HASH_SIZE)

        uint8_t amount_buffer[AMOUNT_BUF_SIZE];
        const int64_t result = slot(SBUF(amount_buffer), amt_slot);

        // ASSERT_FAILURE_MSG >> Could not dump sfAmount
        ASSERT(result == AMOUNT_BUF_SIZE);

        const int64_t float_amt = slot_float(amt_slot);

        // ASSERT_FAILURE_MSG >> Could not parse amount.
        ASSERT(float_amt >= 0);

        // Currency should be EVR.
        // ASSERT_FAILURE_MSG >> Currency should be EVR for host registration.
        ASSERT(IS_EVR(amount_buffer, issuer_accid))

        // Checking whether this host has an initiated transfer to continue.
        TRANSFEREE_ADDR_KEY(account_field);
        uint8_t transferee_addr[TRANSFEREE_ADDR_VAL_SIZE];
        const int has_initiated_transfer = (state_foreign(SBUF(transferee_addr), SBUF(STP_TRANSFEREE_ADDR), FOREIGN_REF) > 0);
        // Check whether host was a transferer of the transfer. (same account continuation).
        const int parties_are_similar = (has_initiated_transfer && BUFFER_EQUAL_20((uint8_t *)(transferee_addr + TRANSFER_HOST_ADDRESS_OFFSET), account_field));

        // Take the host reg fee from config.
        uint64_t host_reg_fee;
        GET_CONF_VALUE(host_reg_fee, STK_HOST_REG_FEE, "Evernode: Could not get host reg fee state.");

        const int64_t comparison_status = (has_initiated_transfer == 0) ? float_compare(float_amt, float_set(0, host_reg_fee), COMPARE_LESS) : float_compare(float_amt, float_set(0, NOW_IN_EVRS), COMPARE_LESS);

        // ASSERT_FAILURE_MSG >> Amount sent is less than the minimum fee for host registration.
        ASSERT(comparison_status != 1)

        // Checking whether this host is already registered.
        HOST_ADDR_KEY(account_field);

        // ASSERT_FAILURE_MSG >> Host already registered.
        ASSERT(!((has_initiated_transfer == 0 || (has_initiated_transfer == 1 && !parties_are_similar)) && state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) != DOESNT_EXIST));

        // <country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><no_of_total_instances(4)><cpu_model(40)><cpu_count(2)><cpu_speed(2)><description(26)><email_address(40)><host_lease_amount(8,xfl)>
        // Populate values to the state address buffer and set state.
        // Clear reserve and description sections first.
        COPY_2BYTES((host_addr + HOST_COUNTRY_CODE_OFFSET), (event_data + HOST_COUNTRY_CODE_PARAM_OFFSET));
        COPY_DESCRIPTION((host_addr + HOST_DESCRIPTION_OFFSET), (event_data + HOST_DESCRIPTION_PARAM_OFFSET));
        INT64_TO_BUF_LE(&host_addr[HOST_REG_LEDGER_OFFSET], cur_ledger_seq);
        UINT64_TO_BUF_LE(&host_addr[HOST_REG_FEE_OFFSET], host_reg_fee);
        COPY_4BYTES((host_addr + HOST_TOT_INS_COUNT_OFFSET), (event_data + HOST_TOT_INS_COUNT_PARAM_OFFSET));
        UINT64_TO_BUF_LE(&host_addr[HOST_REG_TIMESTAMP_OFFSET], cur_ledger_timestamp);
        COPY_8BYTES(&host_addr[HOST_LEASE_AMOUNT_OFFSET], (event_data + HOST_LEASE_AMOUNT_PARAM_OFFSET));

        // Set host reputation based on lease amount and instance count.
        uint32_t min_instance_count = UINT32_FROM_BUF_LE(&reward_configuration[HOST_MIN_INSTANCE_COUNT_OFFSET]);
        int64_t max_lease_amount = INT64_FROM_BUF_LE(&reward_info[HOST_MAX_LEASE_AMOUNT_OFFSET]);
        const int64_t host_lease_amount = INT64_FROM_BUF_LE(&host_addr[HOST_LEASE_AMOUNT_OFFSET]);
        const uint32_t host_instance_count = UINT32_FROM_BUF_LE(&host_addr[HOST_TOT_INS_COUNT_OFFSET]);
        // If reputation conditions aren't met, Make reputation to 0 in both fresh and transfer install.
        if (float_compare(host_lease_amount, float_set(0, 0), COMPARE_EQUAL) == 1 || float_compare(host_lease_amount, max_lease_amount, COMPARE_GREATER) == 1 || host_instance_count < min_instance_count)
            host_addr[HOST_REPUTATION_OFFSET] = 0;
        else
            host_addr[HOST_REPUTATION_OFFSET] = reward_configuration[HOST_REPUTATION_THRESHOLD_OFFSET];

        if (has_initiated_transfer == 0)
        {
            // Continuation of normal registration flow...

            // Generate the NFT token id.

            // Take the account token sequence from keylet.
            uint8_t keylet[34];

            // ASSERT_FAILURE_MSG >> Could not generate the keylet for KEYLET_ACCOUNT.
            ASSERT(util_keylet(SBUF(keylet), KEYLET_ACCOUNT, SBUF(hook_accid), 0, 0, 0, 0) == 34);

            const int64_t slot_no = slot_set(SBUF(keylet), 0);

            // ASSERT_FAILURE_MSG >> Could not set keylet in slot
            ASSERT(slot_no >= 0);

            // Access registry hook account txn sequence.
            uint8_t txid_ref_buf[16];
            CAST_4BYTES_TO_HEXSTR(txid_ref_buf, txid);
            CAST_4BYTES_TO_HEXSTR((txid_ref_buf + 8), (txid + 28));
            uint8_t uri_token_id[URI_TOKEN_ID_SIZE];
            uint8_t uri_hex[REG_URI_TOKEN_URI_SIZE];
            COPY_4BYTES(uri_hex, EVR_HOST);
            COPY_2BYTES((uri_hex + 4), (EVR_HOST + 4));
            COPY_BYTE((uri_hex + 6), (EVR_HOST + 6));
            COPY_16BYTES((uri_hex + 7), txid_ref_buf);
            GENERATE_URI_TOKEN_ID(uri_token_id, hook_accid, uri_hex);

            COPY_32BYTES(host_addr, uri_token_id);

            // ASSERT_FAILURE_MSG >> Could not set state for host_addr.
            ASSERT(state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) >= 0);

            // Populate the values to the token id buffer and set state.
            COPY_20BYTES((token_id + HOST_ADDRESS_OFFSET), account_field);
            COPY_40BYTES((token_id + HOST_CPU_MODEL_NAME_OFFSET), (event_data + HOST_CPU_MODEL_NAME_PARAM_OFFSET));
            COPY_2BYTES((token_id + HOST_CPU_COUNT_OFFSET), (event_data + HOST_CPU_COUNT_PARAM_OFFSET));
            COPY_2BYTES((token_id + HOST_CPU_SPEED_OFFSET), (event_data + HOST_CPU_SPEED_PARAM_OFFSET));
            COPY_4BYTES((token_id + HOST_CPU_MICROSEC_OFFSET), (event_data + HOST_CPU_MICROSEC_PARAM_OFFSET));
            COPY_4BYTES((token_id + HOST_RAM_MB_OFFSET), (event_data + HOST_RAM_MB_PARAM_OFFSET));
            COPY_4BYTES((token_id + HOST_DISK_MB_OFFSET), (event_data + HOST_DISK_MB_PARAM_OFFSET));
            COPY_40BYTES((token_id + HOST_EMAIL_ADDRESS_OFFSET), (event_data + HOST_EMAIL_ADDRESS_PARAM_OFFSET));
            TOKEN_ID_KEY(uri_token_id);

            // ASSERT_FAILURE_MSG >> Could not set state for token_id.
            ASSERT(state_foreign_set(SBUF(token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) >= 0);

            uint32_t host_count;
            GET_HOST_COUNT(host_count);
            host_count += 1;
            SET_HOST_COUNT(host_count);

            // Take the mint limit value from config.
            uint64_t conf_mint_limit;
            GET_CONF_VALUE(conf_mint_limit, CONF_MINT_LIMIT, "Evernode: Could not get mint limit state.");

            etxn_reserve(2);

            uint8_t emithash[32];
            // Mint the uri token.
            PREPARE_URI_TOKEN_MINT_TX(txid_ref_buf);

            // ASSERT_FAILURE_MSG >> Emitting URIToken mint txn failed
            ASSERT(emit(SBUF(emithash), SBUF(REG_URI_TOKEN_MINT_TX)) >= 0);

            // Amount will be 1.
            PREPARE_URI_TOKEN_SELL_OFFER_TX(1, account_field, uri_token_id);

            // ASSERT_FAILURE_MSG >> Emitting offer txn failed
            ASSERT(emit(SBUF(emithash), SBUF(URI_TOKEN_SELL_OFFER)) >= 0);

            // If maximum theoretical host count reached, halve the registration fee.
            const uint64_t deposit_amount = host_reg_fee * host_count;
            const uint64_t next_epochs_reward_amount = epoch_reward_amount * (epoch_count - epoch);
            const int64_t remaining_reward_amount = float_sum(reward_pool_amount, float_set(0, next_epochs_reward_amount));
            const int64_t circulating_supply = float_sum(float_set(0, conf_mint_limit), float_negate(remaining_reward_amount));
            if ((host_reg_fee / 2) > conf_fixed_reg_fee && float_compare(float_set(0, deposit_amount), float_divide(circulating_supply, float_set(0, 2)), COMPARE_GREATER) == 1)
            {
                host_reg_fee /= 2;
                SET_UINT_STATE_VALUE(host_reg_fee, STK_HOST_REG_FEE, "Evernode: Could not update the state for host reg fee.");
            }

            // PERMIT_MSG >> Host registration successful.
            PERMIT();
        }
        else
        {
            // Continuation of re-registration flow (completion of an existing transfer)...

            uint8_t prev_host_addr[HOST_ADDR_VAL_SIZE];
            HOST_ADDR_KEY((uint8_t *)(transferee_addr + TRANSFER_HOST_ADDRESS_OFFSET));

            // ASSERT_FAILURE_MSG >> Previous host address state not found.
            ASSERT(state_foreign(SBUF(prev_host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) >= 0);

            uint8_t prev_token_id[TOKEN_ID_VAL_SIZE];
            TOKEN_ID_KEY((uint8_t *)(prev_host_addr + HOST_TOKEN_ID_OFFSET));

            // ASSERT_FAILURE_MSG >> Previous host token id state not found.
            ASSERT(state_foreign(SBUF(prev_token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) >= 0);

            // Use the previous NFToken id for this re-reg flow.
            COPY_32BYTES(host_addr, (prev_host_addr + HOST_TOKEN_ID_OFFSET));

            // Copy some of the previous host state figures to the new HOST_ADDR state.
            COPY_8BYTES(&host_addr[HOST_REG_LEDGER_OFFSET], &prev_host_addr[HOST_REG_LEDGER_OFFSET]);
            COPY_8BYTES(&host_addr[HOST_HEARTBEAT_TIMESTAMP_OFFSET], &prev_host_addr[HOST_HEARTBEAT_TIMESTAMP_OFFSET]);
            COPY_8BYTES(&host_addr[HOST_REG_TIMESTAMP_OFFSET], &prev_host_addr[HOST_REG_TIMESTAMP_OFFSET]);
            COPY_8BYTES(&host_addr[HOST_TRANSFER_TIMESTAMP_OFFSET], &prev_host_addr[HOST_TRANSFER_TIMESTAMP_OFFSET]);

            // Reputation is set to threshold above if reputation conditions are met. If so copy the previous reputation.
            if (prev_host_addr[HOST_REPUTATION_OFFSET] != 0 && host_addr[HOST_REPUTATION_OFFSET] == reward_configuration[HOST_REPUTATION_THRESHOLD_OFFSET])
                COPY_BYTE(&host_addr[HOST_REPUTATION_OFFSET], &prev_host_addr[HOST_REPUTATION_OFFSET]);
            // Host flags will be copied from previous state.
            COPY_BYTE(&host_addr[HOST_FLAGS_OFFSET], &prev_host_addr[HOST_FLAGS_OFFSET]);

            // Set the STP_HOST_ADDR with corresponding new state's key.
            HOST_ADDR_KEY(account_field);

            // ASSERT_FAILURE_MSG >> Could not set state for host_addr.
            ASSERT(state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) >= 0);

            // Update previous TOKEN_ID state entry with the new attributes.
            COPY_20BYTES((prev_token_id + HOST_ADDRESS_OFFSET), account_field);
            COPY_40BYTES((prev_token_id + HOST_CPU_MODEL_NAME_OFFSET), (event_data + HOST_CPU_MODEL_NAME_PARAM_OFFSET));
            COPY_2BYTES((prev_token_id + HOST_CPU_COUNT_OFFSET), (event_data + HOST_CPU_COUNT_PARAM_OFFSET));
            COPY_2BYTES((prev_token_id + HOST_CPU_SPEED_OFFSET), (event_data + HOST_CPU_SPEED_PARAM_OFFSET));
            COPY_4BYTES((prev_token_id + HOST_CPU_MICROSEC_OFFSET), (event_data + HOST_CPU_MICROSEC_PARAM_OFFSET));
            COPY_4BYTES((prev_token_id + HOST_RAM_MB_OFFSET), (event_data + HOST_RAM_MB_PARAM_OFFSET));
            COPY_4BYTES((prev_token_id + HOST_DISK_MB_OFFSET), (event_data + HOST_DISK_MB_PARAM_OFFSET));
            COPY_40BYTES((prev_token_id + HOST_EMAIL_ADDRESS_OFFSET), (event_data + HOST_EMAIL_ADDRESS_PARAM_OFFSET));

            // ASSERT_FAILURE_MSG >> Could not set state for token_id.
            ASSERT(state_foreign_set(SBUF(prev_token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) >= 0)

            etxn_reserve(1);
            // Amount will be 0 (new 1).
            // Create a sell offer for the transferring URIToken.
            PREPARE_URI_TOKEN_SELL_OFFER_TX(1, account_field, (uint8_t *)(prev_host_addr + HOST_TOKEN_ID_OFFSET));
            uint8_t emithash[HASH_SIZE];

            // ASSERT_FAILURE_MSG >> Emitting offer txn failed
            ASSERT(emit(SBUF(emithash), SBUF(URI_TOKEN_SELL_OFFER)) >= 0);

            // Set the STP_HOST_ADDR correctly of the deleting state.
            HOST_ADDR_KEY((uint8_t *)(transferee_addr + TRANSFER_HOST_ADDRESS_OFFSET));

            // Delete previous HOST_ADDR state and the relevant TRANSFEREE_ADDR state entries accordingly.

            // ASSERT_FAILURE_MSG >> Could not delete the previous host state entry.
            ASSERT(parties_are_similar || (state_foreign_set(0, 0, SBUF(STP_HOST_ADDR), FOREIGN_REF) >= 0))

            // ASSERT_FAILURE_MSG >> Could not delete state related to transfer.
            ASSERT(state_foreign_set(0, 0, SBUF(STP_TRANSFEREE_ADDR), FOREIGN_REF) >= 0)

            // PERMIT_MSG >> Host re-registration successful.
            PERMIT();
        }
    }
    else if (op_type == OP_HOST_UPDATE_REG)
    {
        // Msg format.
        // <token_id(32)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><total_instance_count(4)><active_instances(4)><description(26)><version(3)><email(40)><host_lease_amount(8,xfl)>

        // Active instance count is required, If 0 it means there're no active instances.
        COPY_4BYTES((host_addr + HOST_ACT_INS_COUNT_OFFSET), (event_data + HOST_UPDATE_ACT_INS_COUNT_PARAM_OFFSET));

        if (!IS_BUFFER_EMPTY_4((event_data + HOST_UPDATE_CPU_MICROSEC_PARAM_OFFSET)))
        {
            COPY_4BYTES((token_id + HOST_CPU_MICROSEC_OFFSET), (event_data + HOST_UPDATE_CPU_MICROSEC_PARAM_OFFSET));
        }

        if (!IS_BUFFER_EMPTY_4((event_data + HOST_UPDATE_RAM_MB_PARAM_OFFSET)))
        {
            COPY_4BYTES((token_id + HOST_RAM_MB_OFFSET), (event_data + HOST_UPDATE_RAM_MB_PARAM_OFFSET));
        }

        if (!IS_BUFFER_EMPTY_4((event_data + HOST_UPDATE_DISK_MB_PARAM_OFFSET)))
        {
            COPY_4BYTES((token_id + HOST_DISK_MB_OFFSET), (event_data + HOST_UPDATE_DISK_MB_PARAM_OFFSET));
        }

        if (!IS_EMAIL_ADDRESS_EMPTY((event_data + HOST_UPDATE_EMAIL_ADDRESS_PARAM_OFFSET)))
        {
            COPY_40BYTES((token_id + HOST_EMAIL_ADDRESS_OFFSET), (event_data + HOST_UPDATE_EMAIL_ADDRESS_PARAM_OFFSET));
        }

        if (!IS_BUFFER_EMPTY_4((event_data + HOST_UPDATE_TOT_INS_COUNT_PARAM_OFFSET)))
        {
            COPY_4BYTES((host_addr + HOST_TOT_INS_COUNT_OFFSET), (event_data + HOST_UPDATE_TOT_INS_COUNT_PARAM_OFFSET));
        }

        if (!IS_BUFFER_EMPTY_2((event_data + HOST_UPDATE_COUNTRY_CODE_PARAM_OFFSET)))
        {
            COPY_4BYTES((host_addr + HOST_COUNTRY_CODE_OFFSET), (event_data + HOST_UPDATE_COUNTRY_CODE_PARAM_OFFSET));
        }

        if (!IS_DESCRIPTION_EMPTY((event_data + HOST_UPDATE_DESCRIPTION_PARAM_OFFSET)))
        {
            COPY_10BYTES((host_addr + HOST_DESCRIPTION_OFFSET), (event_data + HOST_UPDATE_DESCRIPTION_PARAM_OFFSET));
            COPY_16BYTES((host_addr + HOST_DESCRIPTION_OFFSET + 10), (event_data + HOST_UPDATE_DESCRIPTION_PARAM_OFFSET + 10));
        }

        if (!IS_VERSION_EMPTY((event_data + HOST_UPDATE_VERSION_PARAM_OFFSET)))
        {
            COPY_BYTE((host_addr + HOST_VERSION_OFFSET), (event_data + HOST_UPDATE_VERSION_PARAM_OFFSET));
            COPY_BYTE((host_addr + HOST_VERSION_OFFSET + 1), (event_data + HOST_UPDATE_VERSION_PARAM_OFFSET + 1));
            COPY_BYTE((host_addr + HOST_VERSION_OFFSET + 2), (event_data + HOST_UPDATE_VERSION_PARAM_OFFSET + 2));
        }

        if (!IS_BUFFER_EMPTY_8((event_data + HOST_UPDATE_LEASE_AMOUNT_PARAM_OFFSET)))
        {
            COPY_8BYTES((host_addr + HOST_LEASE_AMOUNT_OFFSET), (event_data + HOST_UPDATE_LEASE_AMOUNT_PARAM_OFFSET));
        }

        // ASSERT_FAILURE_MSG >> Could not set state for info update.
        ASSERT(!(state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0 || state_foreign_set(SBUF(token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) < 0));

        // PERMIT_MSG >> Update host info successful.
        PERMIT();
    }
    else if (op_type == OP_HOST_REBATE)
    {
        uint64_t reg_fee = UINT64_FROM_BUF_LE(&host_addr[HOST_REG_FEE_OFFSET]);

        uint64_t host_reg_fee;
        GET_CONF_VALUE(host_reg_fee, STK_HOST_REG_FEE, "Evernode: Could not get host reg fee state.");

        // ASSERT_FAILURE_MSG >> Rollback as there are no pending rebates for the host.
        ASSERT(reg_fee > host_reg_fee);

        // Reserve for a transaction emission.
        etxn_reserve(1);

        // Create the outgoing hosting token txn.
        PREPARE_PAYMENT_TRUSTLINE_TX(EVR_TOKEN, issuer_accid, float_set(0, (reg_fee - host_reg_fee)), account_field);
        uint8_t emithash[32];

        // ASSERT_FAILURE_MSG >> Emitting EVR rebate txn failed
        ASSERT(emit(SBUF(emithash), SBUF(PAYMENT_TRUSTLINE)) >= 0);

        // Updating the current reg fee in the host state.
        UINT64_TO_BUF_LE(&host_addr[HOST_REG_FEE_OFFSET], host_reg_fee);

        // ASSERT_FAILURE_MSG >> Could not update host address state.
        ASSERT(state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) >= 0);

        // PERMIT_MSG >> Host rebate successful.
        PERMIT();
    }
    else if (op_type == OP_HOST_TRANSFER)
    {
        if (reserved == STRONG_HOOK)
        {
            // Check for registration entry, if transferee is different from transfer (transferring to a new account).
            if (!BUFFER_EQUAL_20(event_data, account_field))
            {
                HOST_ADDR_KEY(event_data); // Generate account key for transferee.
                uint8_t reg_entry_buf[HOST_ADDR_VAL_SIZE];

                // ASSERT_FAILURE_MSG >> New transferee also a registered host.
                ASSERT(state_foreign(SBUF(reg_entry_buf), SBUF(STP_HOST_ADDR), FOREIGN_REF) == DOESNT_EXIST);
            }

            uint8_t unique_id[HASH_SIZE] = {0};
            // Check if there is dud host removal request
            uint8_t candidate_id[CANDIDATE_ID_VAL_SIZE];
            GET_DUD_HOST_CANDIDATE_ID(account_field, unique_id);
            CANDIDATE_ID_KEY(unique_id);

            if (state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) >= 0)
            {
                etxn_reserve(1);

                COPY_32BYTES(candidate_remove_data, unique_id);
                candidate_remove_data[32] = CANDIDATE_ELECTED;

                // Prepare MIN XRP trigger transaction to governor about transferring the host linked to a dud host candidate.
                PREPARE_REMOVE_CASCADE_CANDIDATE_MIN_PAYMENT(1, state_hook_accid, LINKED_CANDIDATE_REMOVE, candidate_remove_data);

                uint8_t emithash[HASH_SIZE];

                // ASSERT_FAILURE_MSG >> Minimum XRP to governor hook failed.
                ASSERT(emit(SBUF(emithash), SBUF(REMOVE_CASCADE_CANDIDATE_MIN_PAYMENT)) >= 0);
            }

            // Check whether this host has an initiated transfer.
            uint8_t host_transfer_flag = host_addr[HOST_TRANSFER_FLAG_OFFSET];

            // ASSERT_FAILURE_MSG >> Host has a pending transfer.
            ASSERT(host_transfer_flag != PENDING_TRANSFER)

            // Check whether there is an already initiated transfer for the transferee
            TRANSFEREE_ADDR_KEY(event_data);
            // <transferring_host_address(20)><registration_ledger(8)><token_id(32)>
            uint8_t transferee_addr[TRANSFEREE_ADDR_VAL_SIZE];

            // ASSERT_FAILURE_MSG >> There is a previously initiated transfer for this transferee.
            ASSERT(state_foreign(SBUF(transferee_addr), SBUF(STP_TRANSFEREE_ADDR), FOREIGN_REF) == DOESNT_EXIST);

            // Saving the Pending transfer in Hook States.
            COPY_20BYTES((transferee_addr + TRANSFER_HOST_ADDRESS_OFFSET), account_field);
            INT64_TO_BUF_LE(&transferee_addr[TRANSFER_HOST_LEDGER_OFFSET], cur_ledger_seq);
            COPY_32BYTES((transferee_addr + TRANSFER_HOST_TOKEN_ID_OFFSET), (host_addr + HOST_TOKEN_ID_OFFSET));

            // ASSERT_FAILURE_MSG >> Could not set state for transferee_addr.
            ASSERT(state_foreign_set(SBUF(transferee_addr), SBUF(STP_TRANSFEREE_ADDR), FOREIGN_REF) >= 0);

            // Add transfer in progress flag to existing registration record.
            HOST_ADDR_KEY(account_field);
            host_addr[HOST_TRANSFER_FLAG_OFFSET] = TRANSFER_FLAG;
            UINT64_TO_BUF_LE(&host_addr[HOST_TRANSFER_TIMESTAMP_OFFSET], cur_ledger_timestamp);

            // ASSERT_FAILURE_MSG >> Could not set state for host_addr.
            ASSERT(state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) >= 0)

            // Buying the sell offer created by host.
            // This will be bought on a hook_again since this sell offer is not yet validated.

            // ASSERT_FAILURE_MSG >> Hook again failed on update hook.
            ASSERT(hook_again() == 1);

            // PERMIT_MSG >> Successfully updated the transfer data.
            PERMIT();
        }
        else if (reserved == AGAIN_HOOK)
        {
            etxn_reserve(1);

            // Creating the NFT buy offer for 1 XRP drop.
            PREPARE_URI_TOKEN_BUY_TX(1, (uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET));

            uint8_t emithash[HASH_SIZE];

            // ASSERT_FAILURE_MSG >> Emitting buying offer to NFT failed.
            ASSERT(emit(SBUF(emithash), SBUF(URI_TOKEN_BUY_TX)) >= 0);

            // PERMIT_MSG >> Host transfer initiated successfully.
            PERMIT();
        }
    }
    else if (op_type == OP_HOST_UPDATE_REPUTATION)
    {
        // ASSERT_FAILURE_MSG >> Only foundation is allowed to send host reputation update.
        ASSERT(BUFFER_EQUAL_20(foundation_accid, account_field));

        host_addr[HOST_REPUTATION_OFFSET] = event_data[REPUTATION_VALUE_PARAM_OFFSET];

        // ASSERT_FAILURE_MSG >> Could not set state for reputation update.
        ASSERT(!(state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0));

        // PERMIT_MSG >> Update host reputation successful.
        PERMIT();
    }
    else if (op_type == OP_DUD_HOST_REMOVE)
    {
        // ASSERT_FAILURE_MSG >> Only governor is allowed to send dud host remove request.
        ASSERT(BUFFER_EQUAL_20(state_hook_accid, account_field));

        uint8_t unique_id[HASH_SIZE] = {0};
        GET_DUD_HOST_CANDIDATE_ID(event_data, unique_id);
        // <owner_address(20)><candidate_idx(4)><short_name(20)><created_timestamp(8)><proposal_fee(8)><positive_vote_count(4)>
        // <last_vote_timestamp(8)><status(1)><status_change_timestamp(8)><foundation_vote_status(1)><elect_purge_last_try_timestamp(8)><complete_acknowledged(1)>
        uint8_t candidate_id[CANDIDATE_ID_VAL_SIZE];
        CANDIDATE_ID_KEY(unique_id);

        // ASSERT_FAILURE_MSG >> Error getting a candidate for the given id.
        ASSERT(state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) >= 0)

        // ASSERT_FAILURE_MSG >> Trying to remove un-elected dud host.
        ASSERT(candidate_id[CANDIDATE_STATUS_OFFSET] == CANDIDATE_ELECTED)

        // Remove dud host candidate after validation.
        // ASSERT_FAILURE_MSG >> Could not remove dud host candidate.
        ASSERT(state_foreign_set(0, 0, SBUF(STP_CANDIDATE_ID), FOREIGN_REF) >= 0)

        redirect_op_type = OP_HOST_REMOVE;
    }
    else if (op_type == OP_DEAD_HOST_PRUNE)
    {
        int is_prunable = 0;
        IS_HOST_PRUNABLE(host_addr, is_prunable);

        // ASSERT_FAILURE_MSG >> This host is not eligible for forceful removal based on inactiveness.
        ASSERT(is_prunable != 0);

        redirect_op_type = OP_HOST_REMOVE;
    }
    if (op_type == OP_HOST_DEREG || op_type == OP_HOST_DEREG_FROM_REPUTATION)
    {
        uint8_t *token_id_ptr = event_data;
        if (op_type == OP_HOST_DEREG_FROM_REPUTATION)
        {
            // BEGIN: Check for registration entry.
            uint8_t host_acc_keylet[34] = {0};
            util_keylet(SBUF(host_acc_keylet), KEYLET_ACCOUNT, event_data, ACCOUNT_ID_SIZE, 0, 0, 0, 0);

            int64_t cur_slot = 0;
            GET_SLOT_FROM_KEYLET(host_acc_keylet, cur_slot);

            uint8_t wallet_locator[32] = {0};
            GET_SUB_FIELDS(cur_slot, sfWalletLocator, wallet_locator);

            // ASSERT_FAILURE_MSG >> This account is not set as reputation account for this host.
            ASSERT(BUFFER_EQUAL_20((wallet_locator + 1), account_field));

            token_id_ptr = event_data + ACCOUNT_ID_SIZE;
            op_type = OP_HOST_DEREG;
        }

        // ASSERT_FAILURE_MSG >> Token id sent doesn't match with the registered token.
        ASSERT(BUFFER_EQUAL_32(token_id_ptr, (host_addr + HOST_TOKEN_ID_OFFSET)));

        redirect_op_type = OP_HOST_REMOVE;
    }
    // Child hook update trigger.
    else if (op_type == OP_HOOK_UPDATE)
        HANDLE_HOOK_UPDATE(CANDIDATE_REGISTRY_HOOK_HASH_OFFSET);

    if (redirect_op_type == OP_HOST_REMOVE)
    {
        // Reduce host count by 1.
        uint32_t host_count;
        GET_HOST_COUNT(host_count);
        host_count -= 1;
        SET_HOST_COUNT(host_count);

        const uint64_t reg_fee = UINT64_FROM_BUF_LE(&host_addr[HOST_REG_FEE_OFFSET]);

        uint64_t host_reg_fee;
        GET_CONF_VALUE(host_reg_fee, STK_HOST_REG_FEE, "Evernode: Could not get host reg fee state.");

        uint64_t host_rebate_amount = (host_reg_fee / 2) > conf_fixed_reg_fee ? host_reg_fee / 2 : 0;
        uint64_t reward_amount = host_reg_fee > (conf_fixed_reg_fee + host_rebate_amount) ? (host_reg_fee - conf_fixed_reg_fee - host_rebate_amount) : 0;

        // Handle deregistration, If there's no heartbeat after the registration or re-registration.
        if (op_type == OP_HOST_DEREG)
        {
            const uint8_t *heartbeat_ptr = &host_addr[HOST_HEARTBEAT_TIMESTAMP_OFFSET];
            const int64_t last_heartbeat_timestamp = INT64_FROM_BUF_LE(heartbeat_ptr);
            const uint64_t registration_timestamp = UINT64_FROM_BUF_LE(&host_addr[HOST_REG_TIMESTAMP_OFFSET]);
            const uint64_t transfer_timestamp = UINT64_FROM_BUF_LE(&host_addr[HOST_TRANSFER_TIMESTAMP_OFFSET]);
            // If host has been transferred do not allow deregistration until heartbeat.
            if (transfer_timestamp > 0)
            {
                // ASSERT_FAILURE_MSG >> De registration is not allowed until heartbeat for transfers.
                ASSERT(last_heartbeat_timestamp > transfer_timestamp);
            }
            // If hosts hasn't send heartbeat after registration refund in full.
            else if (last_heartbeat_timestamp < registration_timestamp)
            {
                host_rebate_amount = host_reg_fee;
                reward_amount = 0;
            }
        }

        const uint64_t pending_rebate_amount = reg_fee > host_reg_fee ? reg_fee - host_reg_fee : 0;

        const uint64_t total_rebate_amount = host_rebate_amount + pending_rebate_amount;

        const uint8_t *event_type_ptr = op_type == OP_HOST_DEREG ? HOST_DEREG_SELF_RES : (op_type == OP_DEAD_HOST_PRUNE ? DEAD_HOST_PRUNE_RES : DUD_HOST_REMOVE_RES);
        const uint8_t *host_addr_ptr = &token_id[HOST_ADDRESS_OFFSET];

        const uint8_t *host_accumulated_reward_ptr = &token_id[HOST_ACCUMULATED_REWARD_OFFSET];
        const int64_t accumulated_reward = INT64_FROM_BUF_LE(host_accumulated_reward_ptr);
        const int request_reward = float_compare(accumulated_reward, float_set(0, 0), COMPARE_GREATER);
        uint64_t reward_req[ACCOUNT_ID_SIZE + 8];
        if (request_reward == 1)
        {
            COPY_20BYTES(reward_req, &token_id[HOST_ADDRESS_OFFSET]);
            INT64_TO_BUF(&reward_req[ACCOUNT_ID_SIZE], accumulated_reward);
        }

        uint32_t linked_candidate_removal_reserve = 0;
        uint8_t unique_id[HASH_SIZE] = {0};

        uint32_t orphan_candidate_removal_reserve = 0;
        uint8_t candidate_owner[CANDIDATE_OWNER_VAL_SIZE];

        // Get wallet locator if exists.
        // Host account keylet
        uint8_t host_account_keylet[34] = {0};
        util_keylet(SBUF(host_account_keylet), KEYLET_ACCOUNT, host_addr_ptr, 20, 0, 0, 0, 0);

        int64_t cur_slot = slot_set(SBUF(host_account_keylet), 0);
        uint32_t reputation_hook_invoke_reserve = 0;
        uint8_t host_rep_account_id_state_key[32] = {0};
        uint8_t host_reputation_state[24] = {0};
        uint8_t host_rep_account_id[32] = {0};
        if (cur_slot >= 0)
        {
            cur_slot = slot_subfield(cur_slot, sfWalletLocator, 0);
            if (cur_slot >= 0)
            {
                cur_slot = slot(SBUF(host_rep_account_id), cur_slot);
                if (cur_slot >= 0)
                {
                    COPY_20BYTES(host_rep_account_id_state_key + 12, host_rep_account_id);
                    uint32_t reputation_hook_invoke_reserve = (state_foreign(SBUF(host_reputation_state), SBUF(host_rep_account_id_state_key), FOREIGN_REF_CUSTOM(reputation_hook_accid)) != DOESNT_EXIST) ? 1 : 0;
                }
            }
        }

        // Add an additional emission reservation to trigger the governor to remove a dud host candidate, once that candidate related host is deregistered and pruned.
        if (op_type == OP_DEAD_HOST_PRUNE || op_type == OP_HOST_DEREG)
        {
            uint8_t candidate_id[CANDIDATE_ID_VAL_SIZE];

            GET_DUD_HOST_CANDIDATE_ID(host_addr_ptr, unique_id);
            CANDIDATE_ID_KEY(unique_id);

            if (state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) >= 0)
                linked_candidate_removal_reserve += 1;
        }

        // Add an additional emission reservation to trigger the governor to remove the new hook candidate owned by this account (if such exists).
        CANDIDATE_OWNER_KEY(host_addr_ptr);
        if (state_foreign(SBUF(candidate_owner), SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) >= 0)
            orphan_candidate_removal_reserve += 1;

        uint8_t emithash[HASH_SIZE];

        etxn_reserve(((reward_amount > 0 || request_reward == 1) ? 3 : 2) + (linked_candidate_removal_reserve + orphan_candidate_removal_reserve + reputation_hook_invoke_reserve));

        // Add host_rebate_amount - 5 to the epoch's reward pool. If there are pending rewards reward request will be sent to the heartbeat.
        if (reward_amount > 0)
        {
            const int64_t reward_amount_float = float_set(0, reward_amount);
            ADD_TO_REWARD_POOL(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, reward_amount_float);

            PREPARE_HEARTBEAT_TRIGGER_PAYMENT_TX(reward_amount_float, heartbeat_hook_accid, (request_reward == 1 ? PENDING_REWARDS_REQUEST : UPDATE_REWARD_POOL),
                                                 txid, reward_req);
            // ASSERT_FAILURE_MSG >> EVR funding to heartbeat hook account failed.
            ASSERT(emit(SBUF(emithash), SBUF(HEARTBEAT_TRIGGER_PAYMENT)) >= 0);
        }
        else if (request_reward == 1)
        {
            PREPARE_HEARTBEAT_TRIGGER_MIN_PAYMENT_TX(1, heartbeat_hook_accid, PENDING_REWARDS_REQUEST, txid, reward_req);
            // ASSERT_FAILURE_MSG >> EVR funding to heartbeat hook account failed.
            ASSERT(emit(SBUF(emithash), SBUF(HEARTBEAT_TRIGGER_MIN_PAYMENT)) >= 0);
        }

        if (total_rebate_amount > 0)
        {
            // We should rollback if calculated rebate amount is greater than the received registration fee.
            // ASSERT_FAILURE_MSG >> Rebate amount is greater than received.
            ASSERT(total_rebate_amount <= reg_fee);

            // Prepare transaction to send 50% of reg fee and pending rebates to host account.
            PREPARE_REMOVED_HOST_RES_PAYMENT_TX(float_set(0, total_rebate_amount), host_addr_ptr, event_type_ptr, txid);

            // ASSERT_FAILURE_MSG >> Rebating 1/2 reg fee and pending rebates to host account failed.
            ASSERT(emit(SBUF(emithash), SBUF(REMOVED_HOST_RES_PAYMENT)) >= 0);
        }
        else
        {
            // Prepare MIN XRP transaction to host about pruning.
            PREPARE_REMOVED_HOST_RES_MIN_PAYMENT_TX(1, host_addr_ptr, event_type_ptr, txid);

            // ASSERT_FAILURE_MSG >> Minimum XRP to host account failed.
            ASSERT(emit(SBUF(emithash), SBUF(REMOVED_HOST_RES_MIN_PAYMENT)) >= 0);
        }

        // Burn Registration URI token.
        PREPARE_URI_TOKEN_BURN_TX(&host_addr[HOST_TOKEN_ID_OFFSET]);

        // ASSERT_FAILURE_MSG >> Emitting URI token burn txn failed
        ASSERT(emit(SBUF(emithash), SBUF(URI_TOKEN_BURN_TX)) >= 0);

        // Remove reputation states if there are any.
        if (reputation_hook_invoke_reserve > 0)
        {
            // Removing host [REPUTATION_ACC_ID] based state.
            state_foreign_set(0, 0, SBUF(host_rep_account_id_state_key), FOREIGN_REF_CUSTOM(reputation_hook_accid));

            uint8_t removing_moment_rep_accid_state_key[32] = {0};
            uint8_t removing_moment_order_id_state_key[32] = {0};
            uint8_t order_id[8] = {0};

            uint64_t removing_moment = UINT64_FROM_BUF_LE(&host_reputation_state[0]);

            // Removing host [MOMENT + REPUTATION_ACC_ID] based state of before_previous_moment.
            UINT64_TO_BUF_LE(&removing_moment_rep_accid_state_key[4], removing_moment);
            COPY_20BYTES(removing_moment_rep_accid_state_key + 12, host_rep_account_id);
            state_foreign(SBUF(order_id), SBUF(removing_moment_rep_accid_state_key), FOREIGN_REF_CUSTOM(reputation_hook_accid));
            state_foreign_set(0, 0, SBUF(removing_moment_rep_accid_state_key), FOREIGN_REF_CUSTOM(reputation_hook_accid));

            // Removing host [MOMENT + ORDER_ID] based state of before_previous_moment.
            UINT64_TO_BUF_LE(&removing_moment_order_id_state_key[16], removing_moment);
            COPY_8BYTES(removing_moment_order_id_state_key + 24, order_id);
            state_foreign_set(0, 0, SBUF(removing_moment_order_id_state_key), FOREIGN_REF_CUSTOM(reputation_hook_accid));
        }

        // Invoke Governor to trigger on this condition.
        if (linked_candidate_removal_reserve > 0 || orphan_candidate_removal_reserve > 0)
        {
            if (linked_candidate_removal_reserve > 0)
            {
                COPY_32BYTES(candidate_remove_data, unique_id);
                candidate_remove_data[32] = CANDIDATE_ELECTED;
            }
            else
            {
                GET_NEW_HOOK_CANDIDATE_ID(candidate_owner, CANDIDATE_PROPOSE_KEYLETS_PARAM_OFFSET, candidate_remove_data);
                candidate_remove_data[32] = CANDIDATE_PURGED;
            }

            // Prepare MIN XRP trigger transaction to governor about removing the dud host or new hook candidate.
            PREPARE_REMOVE_CASCADE_CANDIDATE_MIN_PAYMENT(1, state_hook_accid, (linked_candidate_removal_reserve > 0 ? LINKED_CANDIDATE_REMOVE : ORPHAN_CANDIDATE_REMOVE), candidate_remove_data);

            // ASSERT_FAILURE_MSG >> Minimum XRP to governor hook failed.
            ASSERT(emit(SBUF(emithash), SBUF(REMOVE_CASCADE_CANDIDATE_MIN_PAYMENT)) >= 0);
        }

        // If this is a transferer remove the transferee state.
        if (host_addr[HOST_TRANSFER_FLAG_OFFSET] == PENDING_TRANSFER)
        {
            TRANSFEREE_ADDR_KEY(host_addr_ptr);
            // ASSERT_FAILURE_MSG >> Could not delete host transferee entry.
            ASSERT(state_foreign_set(0, 0, SBUF(STP_TRANSFEREE_ADDR), FOREIGN_REF) >= 0);
        }

        // Delete registration entries. If there are pending rewards heartbeat hook will delete the states
        if (request_reward != 1)
        {
            // ASSERT_FAILURE_MSG >> Could not delete host registration entry.
            ASSERT(!(state_foreign_set(0, 0, SBUF(STP_TOKEN_ID), FOREIGN_REF) < 0 || state_foreign_set(0, 0, SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0));
        }

        // Here the PERMIT Macro __LINE__ param. differs in each block. Hence we have to call them separately to figure the scenario.
        if (op_type == OP_HOST_DEREG)
        {
            // PERMIT_MSG >> Host de-registration successful.
            PERMIT();
        }
        else if (op_type == OP_DEAD_HOST_PRUNE)
        {
            // PERMIT_MSG >> Dead host prune successful.
            PERMIT();
        }
        else
        {
            // PERMIT_MSG >> Defected Host remove successful.
            PERMIT();
        }
    }

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
