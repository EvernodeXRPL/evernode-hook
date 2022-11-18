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
        rollback(SBUF("Evernode: Could not get common chain params for chain one."), 1);

    if (common_params[CHAIN_IDX_PARAM_OFFSET] == 1)
    {
        uint8_t *seq_param_ptr = &common_params[CUR_LEDGER_SEQ_PARAM_OFFSET];
        int64_t cur_ledger_seq = INT64_FROM_BUF(seq_param_ptr);
        uint8_t *ts_param_ptr = &common_params[CUR_LEDGER_TIMESTAMP_PARAM_OFFSET];
        int64_t cur_ledger_timestamp = INT64_FROM_BUF(ts_param_ptr);
        unsigned char hook_accid[ACCOUNT_ID_SIZE];
        COPY_BUF(hook_accid, 0, common_params, HOOK_ACCID_PARAM_OFFSET, ACCOUNT_ID_SIZE);
        uint8_t account_field[ACCOUNT_ID_SIZE];
        COPY_BUF(account_field, 0, common_params, ACCOUNT_FIELD_PARAM_OFFSET, ACCOUNT_ID_SIZE);

        uint8_t chain_one_params[CHAIN_ONE_PARAMS_SIZE];
        if (hook_param(SBUF(chain_one_params), SBUF(CHAIN_ONE_PARAMS)) < 0)
            rollback(SBUF("Evernode: Could not get params for chain one."), 1);
        uint8_t amount_buffer[AMOUNT_BUF_SIZE];
        COPY_BUF(amount_buffer, 0, chain_one_params, AMOUNT_BUF_PARAM_OFFSET, AMOUNT_BUF_SIZE);
        uint8_t *float_amt_ptr = &chain_one_params[FLOAT_AMT_PARAM_OFFSET];
        int64_t float_amt = INT64_FROM_BUF(float_amt_ptr);
        uint8_t txid[HASH_SIZE];
        COPY_BUF(txid, 0, chain_one_params, TXID_PARAM_OFFSET, HASH_SIZE);

        // Memos
        uint8_t memos[MAX_MEMO_SIZE];
        int64_t memos_len = otxn_field(SBUF(memos), sfMemos);

        if (!memos_len)
            accept(SBUF("Evernode: No memos found, Unhandled transaction."), 1);

        uint8_t *memo_ptr, *type_ptr, *format_ptr, *data_ptr;
        uint32_t memo_len, type_len, format_len, data_len;
        GET_MEMO(0, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len);

        int64_t amt_drops = float_int(float_amt, 6, 0);
        if (amt_drops < 0)
            rollback(SBUF("Evernode: Could not parse amount."), 1);
        int64_t amt_int = amt_drops / 1000000;

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

            if (float_compare(float_amt, float_set(0, host_reg_fee), COMPARE_LESS) == 1)
                rollback(SBUF("Evernode: Amount sent is less than the minimum fee for host registration."), 1);

            // Checking whether this host is already registered.
            HOST_ADDR_KEY(account_field);

            // <token_id(32)><country_code(2)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
            // <no_of_total_instances(4)><no_of_active_instances(4)><last_heartbeat_index(8)><version(3)><registration_timestamp(8)>
            uint8_t host_addr[HOST_ADDR_VAL_SIZE];
            CLEAR_BUF(host_addr, 0, HOST_ADDR_VAL_SIZE); // Initialize buffer wih 0s

            // <host_address(20)><cpu_model_name(40)><cpu_count(2)><cpu_speed(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)>
            uint8_t token_id[TOKEN_ID_VAL_SIZE];
            CLEAR_BUF(token_id, 0, TOKEN_ID_VAL_SIZE); // Initialize buffer wih 0s

            if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) != DOESNT_EXIST)
                rollback(SBUF("Evernode: Host already registered."), 1);

            // Generate the NFT token id.

            // Take the account token sequence from keylet.
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

            COPY_BUF(host_addr, 0, nft_token_id, 0, NFT_TOKEN_ID_SIZE);
            COPY_BUF(host_addr, HOST_COUNTRY_CODE_OFFSET, data_ptr, 0, COUNTRY_CODE_LEN);

            // Read instance details from the memo.
            // We cannot predict the lengths of the numerical values.
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
            COPY_BUF_NON_CONST_LEN_GUARDM(host_addr, HOST_DESCRIPTION_OFFSET, description_ptr, 0, description_len, DESCRIPTION_LEN, 1, 1);
            INT64_TO_BUF(&host_addr[HOST_REG_LEDGER_OFFSET], cur_ledger_seq);
            UINT64_TO_BUF(&host_addr[HOST_REG_FEE_OFFSET], host_reg_fee);
            UINT32_TO_BUF(&host_addr[HOST_TOT_INS_COUNT_OFFSET], total_ins_count);
            UINT64_TO_BUF(&host_addr[HOST_REG_TIMESTAMP_OFFSET], cur_ledger_timestamp);

            if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                rollback(SBUF("Evernode: Could not set state for host_addr."), 1);

            // Populate the values to the token id buffer and set state.
            COPY_BUF(token_id, HOST_ADDRESS_OFFSET, account_field, 0, ACCOUNT_ID_SIZE);
            COPY_BUF_NON_CONST_LEN_GUARDM(token_id, HOST_CPU_MODEL_NAME_OFFSET, cpu_model_ptr, 0, cpu_model_len, CPU_MODEl_NAME_LEN, 1, 1);
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

            // Take the fixed theoretical maximum registrants value from config.
            uint64_t conf_max_reg;
            GET_CONF_VALUE(conf_max_reg, STK_MAX_REG, "Evernode: Could not get max reg fee state.");
            TRACEVAR(conf_max_reg);

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

            // If maximum theoretical host count reached, halve the registration fee.
            if (host_reg_fee > conf_fixed_reg_fee && host_count > (conf_max_reg / 2))
            {
                uint8_t state_buf[8] = {0};

                host_reg_fee /= 2;
                UINT64_TO_BUF(state_buf, host_reg_fee);
                if (state_set(SBUF(state_buf), SBUF(STK_HOST_REG_FEE)) < 0)
                    rollback(SBUF("Evernode: Could not update the state for host reg fee."), 1);

                conf_max_reg *= 2;
                UINT64_TO_BUF(state_buf, conf_max_reg);
                if (state_set(SBUF(state_buf), SBUF(STK_MAX_REG)) < 0)
                    rollback(SBUF("Evernode: Could not update state for max theoretical registrants."), 1);
            }

            accept(SBUF("Host registration successful."), 0);
        }
    }
    else if (common_params[CHAIN_IDX_PARAM_OFFSET] != 1)
    {
    }

    accept(SBUF("Evernode: Transaction is not handled in Hook Position 1."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
