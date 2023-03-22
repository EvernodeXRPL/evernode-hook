#include "registry.h"

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

    uint8_t state_hook_accid[ACCOUNT_ID_SIZE] = {0};
    if (hook_param(SBUF(state_hook_accid), SBUF(PARAM_STATE_HOOK)) < 0)
        rollback(SBUF("Evernode: Error getting the state hook address from params."), 1);

    uint8_t txid[HASH_SIZE];
    const int32_t txid_len = otxn_id(SBUF(txid), 0);
    if (txid_len < HASH_SIZE)
        rollback(SBUF("Evernode: U transaction id missing."), 1);

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
    if (txn_type == ttPAYMENT || txn_type == ttURI_TOKEN_CREATE_SELL_OFFER)
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
                uint8_t redirect_op_type = OP_NONE;

                if (txn_type == ttPAYMENT)
                {
                    if (amt_slot < 0)
                        rollback(SBUF("Evernode: Could not slot otxn.sfAmount"), 1);

                    int64_t is_xrp = slot_type(amt_slot, 1);
                    if (is_xrp < 0)
                        rollback(SBUF("Evernode: Could not determine sent amount type"), 1);

                    if (is_xrp)
                    {
                        if (EQUAL_HOST_DE_REG(type_ptr, type_len))
                            op_type = OP_HOST_DE_REG;
                        else if (EQUAL_HOST_UPDATE_REG(type_ptr, type_len))
                            op_type = OP_HOST_UPDATE_REG;
                        // Dead Host Prune.
                        else if (EQUAL_DEAD_HOST_PRUNE(type_ptr, type_len))
                            op_type = OP_DEAD_HOST_PRUNE;
                        // Host rebate.
                        else if (EQUAL_HOST_REBATE(type_ptr, type_len))
                            op_type = OP_HOST_REBATE;
                        // Hook update trigger.
                        else if (EQUAL_HOOK_UPDATE(type_ptr, type_len))
                            op_type = OP_HOOK_UPDATE;
                        // Dud host remove.
                        else if (EQUAL_DUD_HOST_REMOVE(type_ptr, type_len))
                            op_type = OP_DUD_HOST_REMOVE;
                    }
                    else // IOU payment.
                    {
                        // Host registration.
                        if (EQUAL_HOST_REG(type_ptr, type_len))
                            op_type = OP_HOST_REG;
                    }
                }
                else if (txn_type == ttURI_TOKEN_CREATE_SELL_OFFER)
                {
                    // Host transfer.
                    if (EQUAL_HOST_TRANSFER(type_ptr, type_len))
                        op_type = OP_HOST_TRANSFER;
                }

                if (op_type != OP_NONE)
                {
                    // Memo format validation.
                    if (op_type != OP_HOST_REBATE && !EQUAL_FORMAT_HEX(format_ptr, format_len))
                        rollback(SBUF("Evernode: Memo format should be hex."), 1);

                    // <token_id(32)><country_code(2)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)><no_of_total_instances(4)><no_of_active_instances(4)>
                    // <last_heartbeat_index(8)><version(3)><registration_timestamp(8)><transfer_flag(1)><last_vote_candidate_idx(4)><support_vote_sent(1)>
                    uint8_t host_addr[HOST_ADDR_VAL_SIZE];
                    // <host_address(20)><cpu_model_name(40)><cpu_count(2)><cpu_speed(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><accumulated_reward_amount(8)>
                    uint8_t token_id[TOKEN_ID_VAL_SIZE];

                    // Common logic for host deregistration, heartbeat, update registration, rebate process and transfer.
                    if (op_type == OP_HOST_UPDATE_REG || op_type == OP_HOST_REBATE ||
                        op_type == OP_HOST_TRANSFER || op_type == OP_DEAD_HOST_PRUNE ||
                        op_type == OP_DUD_HOST_REMOVE || op_type == OP_HOST_DE_REG)
                    {
                        // Generate host account key.
                        if (op_type != OP_DEAD_HOST_PRUNE && op_type != OP_DUD_HOST_REMOVE)
                        {
                            HOST_ADDR_KEY(account_field);
                        }
                        else
                        {
                            HOST_ADDR_KEY(data_ptr);
                        }
                        // Check for registration entry.
                        if (state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) == DOESNT_EXIST)
                            rollback(SBUF("Evernode: This host is not registered."), 1);

                        TOKEN_ID_KEY((uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET)); // Generate token id key.
                        // Check for token id entry.
                        if (state_foreign(SBUF(token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) == DOESNT_EXIST)
                            rollback(SBUF("Evernode: This host is not registered."), 1);

                        // Check the ownership of the token to this user before proceeding.
                        int token_exists;
                        IS_REG_TOKEN_EXIST(((op_type == OP_DEAD_HOST_PRUNE || op_type == OP_DUD_HOST_REMOVE) ? data_ptr : account_field), (host_addr + HOST_TOKEN_ID_OFFSET), token_exists);
                        if (!token_exists)
                            rollback(SBUF("Evernode: Registration URIToken does not exist."), 1);
                    }

                    // <epoch_count(uint8_t)><first_epoch_reward_quota(uint32_t)><epoch_reward_amount(uint32_t)><reward_start_moment(uint32_t)><accumulated_reward_frequency(uint16_t)>
                    uint8_t reward_configuration[REWARD_CONFIGURATION_VAL_SIZE];
                    // <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)>
                    uint8_t reward_info[REWARD_INFO_VAL_SIZE];
                    uint8_t epoch_count = 0;
                    uint32_t first_epoch_reward_quota, epoch_reward_amount = 0;
                    // Take the reward info if deregistration or prune.
                    if (op_type == OP_HOST_DE_REG || op_type == OP_DEAD_HOST_PRUNE || op_type == OP_DUD_HOST_REMOVE)
                    {
                        if (state_foreign(reward_configuration, REWARD_CONFIGURATION_VAL_SIZE, SBUF(CONF_REWARD_CONFIGURATION), FOREIGN_REF) < 0 ||
                            state_foreign(reward_info, REWARD_INFO_VAL_SIZE, SBUF(STK_REWARD_INFO), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not get reward configuration or reward info states."), 1);

                        epoch_count = reward_configuration[EPOCH_COUNT_OFFSET];
                        first_epoch_reward_quota = UINT32_FROM_BUF(&reward_configuration[FIRST_EPOCH_REWARD_QUOTA_OFFSET]);
                        epoch_reward_amount = UINT32_FROM_BUF(&reward_configuration[EPOCH_REWARD_AMOUNT_OFFSET]);
                    }

                    uint8_t issuer_accid[ACCOUNT_ID_SIZE] = {0};
                    uint8_t foundation_accid[ACCOUNT_ID_SIZE] = {0};
                    uint8_t heartbeat_hook_accid[ACCOUNT_ID_SIZE];
                    // States does not exists in initialize transaction.
                    if (state_foreign(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR), FOREIGN_REF) < 0 ||
                        state_foreign(SBUF(foundation_accid), SBUF(CONF_FOUNDATION_ADDR), FOREIGN_REF) < 0 ||
                        state_foreign(SBUF(heartbeat_hook_accid), SBUF(CONF_HEARTBEAT_ADDR), FOREIGN_REF) < 0)
                        rollback(SBUF("Evernode: Could not get issuer, foundation or heartbeat account id."), 1);

                    if (op_type == OP_HOST_REG)
                    {
                        // Get transaction hash(id).
                        uint8_t txid[HASH_SIZE];
                        const int32_t txid_len = otxn_id(SBUF(txid), 0);
                        if (txid_len < HASH_SIZE)
                            rollback(SBUF("Evernode: transaction id missing."), 1);

                        uint8_t amount_buffer[AMOUNT_BUF_SIZE];
                        const int64_t result = slot(SBUF(amount_buffer), amt_slot);
                        if (result != AMOUNT_BUF_SIZE)
                            rollback(SBUF("Evernode: Could not dump sfAmount"), 1);
                        const int64_t float_amt = slot_float(amt_slot);
                        if (float_amt < 0)
                            rollback(SBUF("Evernode: Could not parse amount."), 1);

                        // Currency should be EVR.
                        if (!IS_EVR(amount_buffer, issuer_accid))
                            rollback(SBUF("Evernode: Currency should be EVR for host registration."), 1);

                        // Checking whether this host has an initiated transfer to continue.
                        TRANSFEREE_ADDR_KEY(account_field);
                        uint8_t transferee_addr[TRANSFEREE_ADDR_VAL_SIZE];
                        const int has_initiated_transfer = (state_foreign(SBUF(transferee_addr), SBUF(STP_TRANSFEREE_ADDR), FOREIGN_REF) > 0);
                        // Check whether host was a transferer of the transfer. (same account continuation).
                        const int parties_are_similar = (has_initiated_transfer && BUFFER_EQUAL_20((uint8_t *)(transferee_addr + TRANSFER_HOST_ADDRESS_OFFSET), account_field));

                        // Take the host reg fee from config.
                        int64_t host_reg_fee;
                        GET_CONF_VALUE(host_reg_fee, STK_HOST_REG_FEE, "Evernode: Could not get host reg fee state.");

                        const int64_t comparison_status = (has_initiated_transfer == 0) ? float_compare(float_amt, float_set(0, host_reg_fee), COMPARE_LESS) : float_compare(float_amt, float_set(0, NOW_IN_EVRS), COMPARE_LESS);

                        if (comparison_status == 1)
                            rollback(SBUF("Evernode: Amount sent is less than the minimum fee for host registration."), 1);

                        // Checking whether this host is already registered.
                        HOST_ADDR_KEY(account_field);

                        if ((has_initiated_transfer == 0 || (has_initiated_transfer == 1 && !parties_are_similar)) && state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) != DOESNT_EXIST)
                            rollback(SBUF("Evernode: Host already registered."), 1);

                        // <country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><no_of_total_instances(4)><cpu_model(40)><cpu_count(2)><cpu_speed(2)><description(26)><email_address(40)>
                        // Populate values to the state address buffer and set state.
                        // Clear reserve and description sections first.
                        COPY_2BYTES((host_addr + HOST_COUNTRY_CODE_OFFSET), (data_ptr + HOST_COUNTRY_CODE_MEMO_OFFSET));
                        CLEAR_8BYTES((host_addr + HOST_RESERVED_OFFSET));
                        COPY_DESCRIPTION((host_addr + HOST_DESCRIPTION_OFFSET), (data_ptr + HOST_DESCRIPTION_MEMO_OFFSET));
                        INT64_TO_BUF(&host_addr[HOST_REG_LEDGER_OFFSET], cur_ledger_seq);
                        UINT64_TO_BUF(&host_addr[HOST_REG_FEE_OFFSET], host_reg_fee);
                        COPY_4BYTES((host_addr + HOST_TOT_INS_COUNT_OFFSET), (data_ptr + HOST_TOT_INS_COUNT_MEMO_OFFSET));
                        UINT64_TO_BUF(&host_addr[HOST_REG_TIMESTAMP_OFFSET], cur_ledger_timestamp);

                        if (has_initiated_transfer == 0)
                        {
                            // Continuation of normal registration flow...

                            // Generate the NFT token id.

                            // Take the account token sequence from keylet.
                            uint8_t keylet[34];
                            if (util_keylet(SBUF(keylet), KEYLET_ACCOUNT, SBUF(hook_accid), 0, 0, 0, 0) != 34)
                                rollback(SBUF("Evernode: Could not generate the keylet for KEYLET_ACCOUNT."), 10);

                            const int64_t slot_no = slot_set(SBUF(keylet), 0);
                            if (slot_no < 0)
                                rollback(SBUF("Evernode: Could not set keylet in slot"), 10);

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

                            if (state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for host_addr."), 1);

                            // Populate the values to the token id buffer and set state.
                            COPY_20BYTES((token_id + HOST_ADDRESS_OFFSET), account_field);
                            COPY_40BYTES((token_id + HOST_CPU_MODEL_NAME_OFFSET), (data_ptr + HOST_CPU_MODEL_NAME_MEMO_OFFSET));
                            COPY_2BYTES((token_id + HOST_CPU_COUNT_OFFSET), (data_ptr + HOST_CPU_COUNT_MEMO_OFFSET));
                            COPY_2BYTES((token_id + HOST_CPU_SPEED_OFFSET), (data_ptr + HOST_CPU_SPEED_MEMO_OFFSET));
                            COPY_4BYTES((token_id + HOST_CPU_MICROSEC_OFFSET), (data_ptr + HOST_CPU_MICROSEC_MEMO_OFFSET));
                            COPY_4BYTES((token_id + HOST_RAM_MB_OFFSET), (data_ptr + HOST_RAM_MB_MEMO_OFFSET));
                            COPY_4BYTES((token_id + HOST_DISK_MB_OFFSET), (data_ptr + HOST_DISK_MB_MEMO_OFFSET));
                            COPY_40BYTES((token_id + HOST_EMAIL_ADDRESS_OFFSET), (data_ptr + HOST_EMAIL_ADDRESS_MEMO_OFFSET));
                            TOKEN_ID_KEY(uri_token_id);

                            if (state_foreign_set(SBUF(token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for token_id."), 1);

                            uint32_t host_count;
                            GET_HOST_COUNT(host_count);
                            host_count += 1;
                            SET_HOST_COUNT(host_count);

                            // Take the fixed reg fee from config.
                            int64_t conf_fixed_reg_fee;
                            GET_CONF_VALUE(conf_fixed_reg_fee, CONF_FIXED_REG_FEE, "Evernode: Could not get fixed reg fee state.");

                            // Take the fixed theoretical maximum registrants value from config.
                            uint64_t conf_max_reg;
                            GET_CONF_VALUE(conf_max_reg, STK_MAX_REG, "Evernode: Could not get max reg fee state.");

                            etxn_reserve(3);

                            uint8_t emithash[32];
                            // Froward 5 EVRs to foundation.
                            // Create the outgoing hosting token txn.
                            PREPARE_PAYMENT_TRUSTLINE_TX(EVR_TOKEN, issuer_accid, float_set(0, conf_fixed_reg_fee), foundation_accid);

                            if (emit(SBUF(emithash), SBUF(PAYMENT_TRUSTLINE)) < 0)
                                rollback(SBUF("Evernode: Emitting EVR forward txn failed"), 1);
                            trace(SBUF("emit hash: "), SBUF(emithash), 1);

                            // Mint the uri token.
                            PREPARE_URI_TOKEN_MINT_TX(txid_ref_buf);
                            if (emit(SBUF(emithash), SBUF(REG_URI_TOKEN_MINT_TX)) < 0)
                                rollback(SBUF("Evernode: Emitting URIToken mint txn failed"), 1);
                            trace(SBUF("emit hash: "), SBUF(emithash), 1);

                            // Amount will be 1.
                            PREPARE_URI_TOKEN_SELL_OFFER_TX(1, account_field, uri_token_id);

                            if (emit(SBUF(emithash), SBUF(URI_TOKEN_SELL_OFFER)) < 0)
                                rollback(SBUF("Evernode: Emitting offer txn failed"), 1);
                            trace(SBUF("emit hash: "), SBUF(emithash), 1);

                            // If maximum theoretical host count reached, halve the registration fee.
                            if (host_reg_fee > conf_fixed_reg_fee && host_count >= (conf_max_reg / 2))
                            {
                                uint8_t state_buf[8] = {0};

                                host_reg_fee /= 2;
                                UINT64_TO_BUF(state_buf, host_reg_fee);
                                if (state_foreign_set(SBUF(state_buf), SBUF(STK_HOST_REG_FEE), FOREIGN_REF) < 0)
                                    rollback(SBUF("Evernode: Could not update the state for host reg fee."), 1);

                                conf_max_reg *= 2;
                                UINT64_TO_BUF(state_buf, conf_max_reg);
                                if (state_foreign_set(SBUF(state_buf), SBUF(STK_MAX_REG), FOREIGN_REF) < 0)
                                    rollback(SBUF("Evernode: Could not update state for max theoretical registrants."), 1);
                            }

                            accept(SBUF("Host registration successful."), 0);
                        }
                        else
                        {
                            // Continuation of re-registration flow (completion of an existing transfer)...

                            uint8_t prev_host_addr[HOST_ADDR_VAL_SIZE];
                            HOST_ADDR_KEY((uint8_t *)(transferee_addr + TRANSFER_HOST_ADDRESS_OFFSET));
                            if (state_foreign(SBUF(prev_host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Previous host address state not found."), 1);

                            uint8_t prev_token_id[TOKEN_ID_VAL_SIZE];
                            TOKEN_ID_KEY((uint8_t *)(prev_host_addr + HOST_TOKEN_ID_OFFSET));
                            if (state_foreign(SBUF(prev_token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Previous host token id state not found."), 1);

                            // Use the previous NFToken id for this re-reg flow.
                            COPY_32BYTES(host_addr, (prev_host_addr + HOST_TOKEN_ID_OFFSET));

                            // Copy some of the previous host state figures to the new HOST_ADDR state.
                            const uint8_t *heartbeat_ptr = &prev_host_addr[HOST_HEARTBEAT_TIMESTAMP_OFFSET];
                            INT64_TO_BUF(&host_addr[HOST_REG_LEDGER_OFFSET], INT64_FROM_BUF(heartbeat_ptr));
                            UINT64_TO_BUF(&host_addr[HOST_HEARTBEAT_TIMESTAMP_OFFSET], UINT64_FROM_BUF(prev_host_addr + HOST_HEARTBEAT_TIMESTAMP_OFFSET));
                            UINT64_TO_BUF(&host_addr[HOST_REG_TIMESTAMP_OFFSET], UINT64_FROM_BUF(prev_host_addr + HOST_REG_TIMESTAMP_OFFSET));

                            // Set the STP_HOST_ADDR with corresponding new state's key.
                            HOST_ADDR_KEY(account_field);

                            if (state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for host_addr."), 1);

                            // Update previous TOKEN_ID state entry with the new attributes.
                            COPY_20BYTES((prev_token_id + HOST_ADDRESS_OFFSET), account_field);
                            COPY_40BYTES((prev_token_id + HOST_CPU_MODEL_NAME_OFFSET), (data_ptr + HOST_CPU_MODEL_NAME_MEMO_OFFSET));
                            COPY_2BYTES((prev_token_id + HOST_CPU_COUNT_OFFSET), (data_ptr + HOST_CPU_COUNT_MEMO_OFFSET));
                            COPY_2BYTES((prev_token_id + HOST_CPU_SPEED_OFFSET), (data_ptr + HOST_CPU_SPEED_MEMO_OFFSET));
                            COPY_4BYTES((prev_token_id + HOST_CPU_MICROSEC_OFFSET), (data_ptr + HOST_CPU_MICROSEC_MEMO_OFFSET));
                            COPY_4BYTES((prev_token_id + HOST_RAM_MB_OFFSET), (data_ptr + HOST_RAM_MB_MEMO_OFFSET));
                            COPY_4BYTES((prev_token_id + HOST_DISK_MB_OFFSET), (data_ptr + HOST_DISK_MB_MEMO_OFFSET));
                            COPY_40BYTES((prev_token_id + HOST_EMAIL_ADDRESS_OFFSET), (data_ptr + HOST_EMAIL_ADDRESS_MEMO_OFFSET));

                            if (state_foreign_set(SBUF(prev_token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for token_id."), 1);

                            etxn_reserve(1);
                            // Amount will be 0 (new 1).
                            // Create a sell offer for the transferring URIToken.
                            PREPARE_URI_TOKEN_SELL_OFFER_TX(1, account_field, (uint8_t *)(prev_host_addr + HOST_TOKEN_ID_OFFSET));
                            uint8_t emithash[HASH_SIZE];
                            if (emit(SBUF(emithash), SBUF(URI_TOKEN_SELL_OFFER)) < 0)
                                rollback(SBUF("Evernode: Emitting offer txn failed"), 1);
                            trace(SBUF("emit hash: "), SBUF(emithash), 1);

                            // Set the STP_HOST_ADDR correctly of the deleting state.
                            HOST_ADDR_KEY((uint8_t *)(transferee_addr + TRANSFER_HOST_ADDRESS_OFFSET));

                            // Delete previous HOST_ADDR state and the relevant TRANSFEREE_ADDR state entries accordingly.

                            if (!parties_are_similar && (state_foreign_set(0, 0, SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0))
                                rollback(SBUF("Evernode: Could not delete the previous host state entry."), 1);

                            if (state_foreign_set(0, 0, SBUF(STP_TRANSFEREE_ADDR), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not delete state related to transfer."), 1);

                            accept(SBUF("Host re-registration successful."), 0);
                        }
                    }
                    else if (op_type == OP_HOST_UPDATE_REG)
                    {
                        // Msg format.
                        // <token_id(32)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><total_instance_count(4)><active_instances(4)><description(26)><version(3)>
                        // All data fields are optional in update info transaction. Update state only if an information update is detected.
                        int is_updated = 0;

                        if (!IS_BUFFER_EMPTY_4((data_ptr + HOST_UPDATE_CPU_MICROSEC_MEMO_OFFSET)))
                        {
                            COPY_4BYTES((token_id + HOST_CPU_MICROSEC_OFFSET), (data_ptr + HOST_UPDATE_CPU_MICROSEC_MEMO_OFFSET));
                            is_updated = 1;
                        }

                        if (!IS_BUFFER_EMPTY_4((data_ptr + HOST_UPDATE_RAM_MB_MEMO_OFFSET)))
                        {
                            COPY_4BYTES((token_id + HOST_RAM_MB_OFFSET), (data_ptr + HOST_UPDATE_RAM_MB_MEMO_OFFSET));
                            is_updated = 1;
                        }

                        if (!IS_BUFFER_EMPTY_4((data_ptr + HOST_UPDATE_DISK_MB_MEMO_OFFSET)))
                        {
                            COPY_4BYTES((token_id + HOST_DISK_MB_OFFSET), (data_ptr + HOST_UPDATE_DISK_MB_MEMO_OFFSET));
                            is_updated = 1;
                        }

                        if (!IS_BUFFER_EMPTY_4((data_ptr + HOST_UPDATE_TOT_INS_COUNT_MEMO_OFFSET)))
                        {
                            COPY_4BYTES((host_addr + HOST_TOT_INS_COUNT_OFFSET), (data_ptr + HOST_UPDATE_TOT_INS_COUNT_MEMO_OFFSET));
                            is_updated = 1;
                        }

                        if (!IS_BUFFER_EMPTY_4((data_ptr + HOST_UPDATE_ACT_INS_COUNT_MEMO_OFFSET)))
                        {
                            COPY_4BYTES((host_addr + HOST_ACT_INS_COUNT_OFFSET), (data_ptr + HOST_UPDATE_ACT_INS_COUNT_MEMO_OFFSET));
                            is_updated = 1;
                        }

                        if (!IS_VERSION_EMPTY((data_ptr + HOST_UPDATE_VERSION_MEMO_OFFSET)))
                        {
                            COPY_BYTE((host_addr + HOST_VERSION_OFFSET), (data_ptr + HOST_UPDATE_VERSION_MEMO_OFFSET));
                            COPY_BYTE((host_addr + HOST_VERSION_OFFSET + 1), (data_ptr + HOST_UPDATE_VERSION_MEMO_OFFSET + 1));
                            COPY_BYTE((host_addr + HOST_VERSION_OFFSET + 2), (data_ptr + HOST_UPDATE_VERSION_MEMO_OFFSET + 2));
                            is_updated = 1;
                        }

                        if (is_updated)
                        {
                            if (state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0 || state_foreign_set(SBUF(token_id), SBUF(STP_TOKEN_ID), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for info update."), 1);
                        }

                        accept(SBUF("Evernode: Update host info successful."), 0);
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

                            // Create the outgoing hosting token txn.
                            PREPARE_PAYMENT_TRUSTLINE_TX(EVR_TOKEN, issuer_accid, float_set(0, (reg_fee - host_reg_fee)), account_field);
                            uint8_t emithash[32];
                            if (emit(SBUF(emithash), SBUF(PAYMENT_TRUSTLINE)) < 0)
                                rollback(SBUF("Evernode: Emitting EVR rebate txn failed"), 1);
                            trace(SBUF("emit hash: "), SBUF(emithash), 1);

                            // Updating the current reg fee in the host state.
                            UINT64_TO_BUF(&host_addr[HOST_REG_FEE_OFFSET], host_reg_fee);
                            if (state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not update host address state."), 1);
                        }
                        else
                            rollback(SBUF("Evernode: No pending rebates for the host."), 1);

                        accept(SBUF("Evernode: Host rebate successful."), 0);
                    }
                    else if (op_type == OP_HOST_TRANSFER)
                    {
                        if (reserved == STRONG_HOOK)
                        {
                            // Check for registration entry, if transferee is different from transfer (transferring to a new account).
                            if (!BUFFER_EQUAL_20(data_ptr, account_field))
                            {
                                HOST_ADDR_KEY(data_ptr); // Generate account key for transferee.
                                uint8_t reg_entry_buf[HOST_ADDR_VAL_SIZE];
                                if (state_foreign(SBUF(reg_entry_buf), SBUF(STP_HOST_ADDR), FOREIGN_REF) != DOESNT_EXIST)
                                    rollback(SBUF("Evernode: New transferee also a registered host."), 1);
                            }

                            // Check whether this host has an initiated transfer.
                            uint8_t host_transfer_flag = host_addr[HOST_TRANSFER_FLAG_OFFSET];
                            if (host_transfer_flag == PENDING_TRANSFER)
                                rollback(SBUF("Evernode: Host has a pending transfer."), 1);

                            // Check whether there is an already initiated transfer for the transferee
                            TRANSFEREE_ADDR_KEY(data_ptr);
                            // <transferring_host_address(20)><registration_ledger(8)><token_id(32)>
                            uint8_t transferee_addr[TRANSFEREE_ADDR_VAL_SIZE];

                            if (state_foreign(SBUF(transferee_addr), SBUF(STP_TRANSFEREE_ADDR), FOREIGN_REF) != DOESNT_EXIST)
                                rollback(SBUF("Evernode: There is a previously initiated transfer for this transferee."), 1);

                            // Saving the Pending transfer in Hook States.
                            COPY_20BYTES((transferee_addr + TRANSFER_HOST_ADDRESS_OFFSET), account_field);
                            INT64_TO_BUF(&transferee_addr[TRANSFER_HOST_LEDGER_OFFSET], cur_ledger_seq);
                            COPY_32BYTES((transferee_addr + TRANSFER_HOST_TOKEN_ID_OFFSET), (host_addr + HOST_TOKEN_ID_OFFSET));

                            if (state_foreign_set(SBUF(transferee_addr), SBUF(STP_TRANSFEREE_ADDR), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for transferee_addr."), 1);

                            // Add transfer in progress flag to existing registration record.
                            HOST_ADDR_KEY(account_field);
                            host_addr[HOST_TRANSFER_FLAG_OFFSET] = TRANSFER_FLAG;

                            if (state_foreign_set(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0)
                                rollback(SBUF("Evernode: Could not set state for host_addr."), 1);

                            // Buying the sell offer created by host.
                            // This will be bought on a hook_again since this sell offer is not yet validated.

                            if (hook_again() != 1)
                                rollback(SBUF("Evernode: Hook again failed on update hook."), 1);

                            accept(SBUF("Evernode: Successfully updated the transfer data."), 0);
                        }
                        else if (reserved == AGAIN_HOOK)
                        {
                            etxn_reserve(1);

                            // Creating the NFT buy offer for 1 XRP drop.
                            PREPARE_URI_TOKEN_BUY_TX(1, (uint8_t *)(host_addr + HOST_TOKEN_ID_OFFSET));

                            uint8_t emithash[HASH_SIZE];
                            if (emit(SBUF(emithash), SBUF(URI_TOKEN_BUY_TX)) < 0)
                                rollback(SBUF("Evernode: Emitting buying offer to NFT failed."), 1);
                            trace(SBUF("emit hash: "), SBUF(emithash), 1);

                            accept(SBUF("Evernode: Host transfer initiated successfully."), 0);
                        }
                    }
                    else if (op_type == OP_DUD_HOST_REMOVE)
                    {
                        if (!BUFFER_EQUAL_20(state_hook_accid, account_field))
                            rollback(SBUF("Evernode: Only governor is allowed to send dud host remove request."), 1);

                        uint8_t unique_id[HASH_SIZE] = {0};
                        GET_DUD_HOST_CANDIDATE_ID(data_ptr, unique_id);
                        // <owner_address(20)><candidate_idx(4)><short_name(20)><created_timestamp(8)><proposal_fee(8)><positive_vote_count(4)>
                        // <last_vote_timestamp(8)><status(1)><status_change_timestamp(8)><foundation_vote_status(1)>
                        uint8_t candidate_id[CANDIDATE_ID_VAL_SIZE];
                        CANDIDATE_ID_KEY(unique_id);
                        if (state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Error getting a candidate for the given id."), 1);

                        if (candidate_id[CANDIDATE_STATUS_OFFSET] != CANDIDATE_ELECTED)
                            rollback(SBUF("Evernode: Trying to remove un-elected dud host."), 1);

                        // Remove dud host candidate after validation.
                        if (state_foreign_set(0, 0, SBUF(STP_CANDIDATE_ID), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not remove dud host candidate."), 1);

                        redirect_op_type = OP_HOST_REMOVE;
                    }
                    else if (op_type == OP_DEAD_HOST_PRUNE)
                    {
                        int is_prunable = 0;
                        IS_HOST_PRUNABLE(host_addr, is_prunable);
                        if (is_prunable == 0)
                            rollback(SBUF("Evernode: This host is not eligible for forceful removal based on inactiveness."), 1);

                        redirect_op_type = OP_HOST_REMOVE;
                    }
                    if (op_type == OP_HOST_DE_REG)
                    {
                        if (!BUFFER_EQUAL_32(data_ptr, (host_addr + HOST_TOKEN_ID_OFFSET)))
                            rollback(SBUF("Evernode: Token id sent doesn't match with the registered token."), 1);

                        redirect_op_type = OP_HOST_REMOVE;
                    }
                    // Child hook update trigger.
                    else if (op_type == OP_HOOK_UPDATE)
                        HANDLE_HOOK_UPDATE(CANDIDATE_REGISTRY_HOOK_HASH_OFFSET);

                    if (redirect_op_type == OP_HOST_REMOVE)
                    {
                        // Reduce host count by 1.
                        uint64_t host_count;
                        GET_HOST_COUNT(host_count);
                        host_count -= 1;
                        SET_HOST_COUNT(host_count);

                        const uint64_t reg_fee = UINT64_FROM_BUF(host_addr + HOST_REG_FEE_OFFSET);
                        uint64_t fixed_reg_fee;
                        GET_CONF_VALUE(fixed_reg_fee, CONF_FIXED_REG_FEE, "Evernode: Could not get fixed reg fee.");

                        uint64_t host_reg_fee;
                        GET_CONF_VALUE(host_reg_fee, STK_HOST_REG_FEE, "Evernode: Could not get host reg fee state.");

                        const uint64_t amount_half = host_reg_fee > fixed_reg_fee ? host_reg_fee / 2 : 0;
                        const uint64_t reward_amount = (amount_half - fixed_reg_fee);
                        const uint64_t pending_rebate_amount = reg_fee > host_reg_fee ? reg_fee - host_reg_fee : 0;

                        const uint64_t total_rebate_amount = amount_half + pending_rebate_amount;

                        const uint8_t *memo_type_ptr = op_type == OP_HOST_DE_REG ? HOST_DE_REG_RES : (op_type == OP_DEAD_HOST_PRUNE ? DEAD_HOST_PRUNE_RES : DUD_HOST_REMOVE_RES);
                        const uint8_t *host_addr_ptr = op_type == OP_HOST_DE_REG ? account_field : data_ptr;

                        const uint8_t *host_accumulated_reward_ptr = &token_id[HOST_ACCUMULATED_REWARD_OFFSET];
                        const int64_t accumulated_reward = INT64_FROM_BUF(host_accumulated_reward_ptr);
                        if (float_compare(accumulated_reward, float_set(0, 0), COMPARE_GREATER) == 1)
                        {
                            // TODO: Send reward trigger to the heartbeat.
                        }

                        uint8_t emithash[HASH_SIZE];
                        etxn_reserve(reward_amount > 0 ? 3 : 2);
                        // Add amount_half - 5 to the epoch's reward pool.
                        if (reward_amount > 0)
                        {
                            const int64_t reward_amount_float = float_set(0, reward_amount);
                            ADD_TO_REWARD_POOL(reward_info, epoch_count, first_epoch_reward_quota, epoch_reward_amount, moment_base_idx, reward_amount_float);

                            PREPARE_HEARTBEAT_FUND_PAYMENT_TX(reward_amount_float, heartbeat_hook_accid, txid);
                            if (emit(SBUF(emithash), SBUF(HEARTBEAT_FUND_PAYMENT)) < 0)
                                rollback(SBUF("Evernode: EVR funding to heartbeat hook account failed."), 1);
                            trace(SBUF("emit hash: "), SBUF(emithash), 1);
                        }

                        if (total_rebate_amount > 0)
                        {
                            // Prepare transaction to send 50% of reg fee and pending rebates to host account.
                            PREPARE_REMOVED_HOST_RES_PAYMENT_TX(float_set(0, total_rebate_amount), host_addr_ptr, memo_type_ptr, txid);
                            if (emit(SBUF(emithash), SBUF(REMOVED_HOST_RES_PAYMENT)) < 0)
                                rollback(SBUF("Evernode: Rebating 1/2 reg fee and pending rebates to host account failed."), 1);
                            trace(SBUF("emit hash: "), SBUF(emithash), 1);
                        }
                        else
                        {
                            // Prepare MIN XRP transaction to host about pruning.
                            PREPARE_REMOVED_HOST_RES_MIN_PAYMENT_TX(1, host_addr_ptr, memo_type_ptr, txid);
                            if (emit(SBUF(emithash), SBUF(REMOVED_HOST_RES_MIN_PAYMENT)) < 0)
                                rollback(SBUF("Evernode: Minimum XRP to host account failed."), 1);
                            trace(SBUF("emit hash: "), SBUF(emithash), 1);
                        }

                        // Burn Registration URI token.
                        PREPARE_URI_TOKEN_BURN_TX(&host_addr[HOST_TOKEN_ID_OFFSET]);
                        if (emit(SBUF(emithash), SBUF(URI_TOKEN_BURN_TX)) < 0)
                            rollback(SBUF("Evernode: Emitting URI token burn txn failed"), 1);
                        trace(SBUF("emit hash: "), SBUF(emithash), 1);

                        // Delete registration entries.
                        if (state_foreign_set(0, 0, SBUF(STP_TOKEN_ID), FOREIGN_REF) < 0 || state_foreign_set(0, 0, SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0)
                            rollback(SBUF("Evernode: Could not delete host registration entry."), 1);

                        if (op_type == OP_HOST_DE_REG)
                            accept(SBUF("Evernode: Host de-registration successful."), 0);
                        else if (op_type == DEAD_HOST_PRUNE)
                            accept(SBUF("Evernode: Dead host prune successful."), 0);
                        else
                            accept(SBUF("Evernode: Defected Host remove successful."), 0);
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
