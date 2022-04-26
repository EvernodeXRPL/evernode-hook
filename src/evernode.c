// #include "../lib/hookapi.h"
#include "../lib/emulatorapi.h"
#include "evernode.h"
#include "statekeys.h"

// Executed when an emitted transaction is successfully accepted into a ledger
// or when an emitted transaction cannot be accepted into any ledger (with what = 1),
int64_t cbak(int64_t reserved)
{
    return 0;
}

// Executed whenever a transaction comes into or leaves from the account the Hook is set on.
int64_t hook(int64_t reserved)
{
    // Getting the hook account id.
    unsigned char hook_accid[20];
    hook_account((uint32_t)hook_accid, 20);

    // Next fetch the sfAccount field from the originating transaction
    uint8_t account_field[20];
    int32_t account_field_len = otxn_field(SBUF(account_field), sfAccount);
    if (account_field_len < 20)
        rollback(SBUF("Evernode: sfAccount field is missing."), 1);

    // Accept any outgoing transactions without further processing.
    int is_outgoing = 0;
    BUFFER_EQUAL(is_outgoing, hook_accid, account_field, 20);
    if (is_outgoing)
        accept(SBUF("Evernode: Outgoing transaction. Passing."), 0);

    // Get transaction hash(id).
    uint8_t txid[HASH_SIZE];
    int32_t txid_len = otxn_id(SBUF(txid), 0);
    if (txid_len < HASH_SIZE)
        rollback(SBUF("Evernode: transaction id missing."), 1);

    int64_t txn_type = otxn_type();
    if (txn_type == ttPAYMENT || txn_type == ttNFT_ACCEPT_OFFER)
    {
        int64_t cur_ledger_seq = ledger_seq();

        // Memos
        uint8_t memos[MAX_MEMO_SIZE];
        int64_t memos_len = otxn_field(SBUF(memos), sfMemos);

        if (!memos_len)
            accept(SBUF("Evernode: No memos found, Unhandled transaction."), 1);

        uint8_t *memo_ptr, *type_ptr, *format_ptr, *data_ptr;
        uint32_t memo_len, type_len, format_len, data_len;
        GET_MEMO(0, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len);

        if (txn_type == ttPAYMENT)
        {
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

                    if (data_len != (2 * ACCOUNT_LEN))
                        rollback(SBUF("Evernode: Memo data should contain foundation cold wallet and issuer addresses."), 1);

                    uint8_t *issuer_ptr = data_ptr;
                    uint8_t *foundation_ptr = data_ptr + ACCOUNT_LEN;

                    uint8_t initializer_accid[ACCOUNT_LEN];
                    const int initializer_accid_len = util_accid(SBUF(initializer_accid), HOOK_INITIALIZER_ADDR, 35);
                    if (initializer_accid_len < ACCOUNT_LEN)
                        rollback(SBUF("Evernode: Could not convert initializer account id."), 1);

                    // We accept only the init transaction from hook intializer account
                    BUFFER_EQUAL(is_initialize, initializer_accid, account_field, ACCOUNT_LEN);
                    if (!is_initialize)
                        rollback(SBUF("Evernode: Only initializer is allowed to initialize state."), 1);

                    // First check if the states are already initialized by checking one state key for existence.
                    uint8_t host_count_buf[8];
                    if (state(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) != DOESNT_EXIST)
                        rollback(SBUF("Evernode: State is already initialized."), 1);

                    // Initialize the state.
                    const uint64_t zero = 0;
                    SET_UINT_STATE_VALUE(zero, STK_HOST_COUNT, "Evernode: Could not initialize state for host count.");
                    SET_UINT_STATE_VALUE(zero, STK_MOMENT_BASE_IDX, "Evernode: Could not initialize state for moment base index.");
                    SET_UINT_STATE_VALUE(DEF_HOST_REG_FEE, STK_HOST_REG_FEE, "Evernode: Could not initialize state for reg fee.");
                    SET_UINT_STATE_VALUE(DEF_MAX_REG, STK_MAX_REG, "Evernode: Could not initialize state for maximum registrants.");

                    if (state_set(issuer_ptr, ACCOUNT_LEN, SBUF(CONF_ISSUER_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not set state for issuer account."), 1);

                    if (state_set(foundation_ptr, ACCOUNT_LEN, SBUF(CONF_FOUNDATION_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not set state for foundation account."), 1);

                    SET_UINT_STATE_VALUE(DEF_MOMENT_SIZE, CONF_MOMENT_SIZE, "Evernode: Could not initialize state for moment size.");
                    SET_UINT_STATE_VALUE(DEF_MINT_LIMIT, CONF_MINT_LIMIT, "Evernode: Could not initialize state for mint limit.");
                    SET_UINT_STATE_VALUE(DEF_FIXED_REG_FEE, CONF_FIXED_REG_FEE, "Evernode: Could not initialize state for fixed reg fee.");
                    SET_UINT_STATE_VALUE(DEF_HOST_HEARTBEAT_FREQ, CONF_HOST_HEARTBEAT_FREQ, "Evernode: Could not initialize state for heartbeat frequency.");
                    SET_UINT_STATE_VALUE(DEF_LEASE_ACQUIRE_WINDOW, CONF_LEASE_ACQUIRE_WINDOW, "Evernode: Could not initialize state for lease acquire window.");

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
                    uint16_t flags;
                    uint8_t issuer[20], uri[256], uri_len;
                    uint32_t taxon;
                    if (get_nft(account_field, reg_entry_buf + HOST_TOKEN_ID_OFFSET, &flags, issuer, &taxon, uri, &uri_len) == DOESNT_EXIST)
                        rollback(SBUF("Evernode: Token mismatch with registration."), 1);

                    // Issuer of the NFT should be the registry contract.
                    int is_issuer_match = 0;
                    BUFFER_EQUAL(is_issuer_match, issuer, hook_accid, 20);
                    if (!is_issuer_match)
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

                    // Sending 50% reg fee to foundation account.
                    int64_t amount_half = reg_fee > fixed_reg_fee ? reg_fee / 2 : 0;

                    if (reg_fee > fixed_reg_fee)
                    {
                        uint8_t amt_out_return[AMOUNT_BUF_SIZE];
                        // Since we have already sent 5 EVR in the registration process to the foundation. We should deduct 5 EVR from the 50% reg fee.
                        SET_AMOUNT_OUT(amt_out_return, EVR_TOKEN, issuer_accid, float_set(0, (amount_half - 5)));
                        etxn_reserve(2);
                        int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                        // Prepare transaction to send 50% of reg fee to foundation account.
                        uint8_t tx_buf[PREPARE_PAYMENT_FOUNDATION_RETURN_SIZE];
                        PREPARE_PAYMENT_FOUNDATION_RETURN(tx_buf, amt_out_return, fee, foundation_accid);

                        uint8_t emithash[HASH_SIZE];
                        if (emit(SBUF(emithash), SBUF(tx_buf)) < 0)
                            rollback(SBUF("Evernode: Emitting 1/2 reg fee to foundation failed."), 1);
                    }
                    else
                        etxn_reserve(1);

                    uint8_t amt_out[AMOUNT_BUF_SIZE];
                    SET_AMOUNT_OUT(amt_out, EVR_TOKEN, issuer_accid, float_set(0, amount_half));
                    // Creating the NFT buying offer. If he has paid more than fixed reg fee, we create buy offer to reg_fee/2. If not, for 0 EVR.
                    uint8_t buy_tx_buf[PREPARE_NFT_BUY_OFFER_SIZE];
                    int64_t buy_fee = etxn_fee_base(PREPARE_NFT_BUY_OFFER_SIZE);
                    PREPARE_NFT_BUY_OFFER(buy_tx_buf, amt_out, buy_fee, account_field, (uint8_t *)(reg_entry_buf + HOST_TOKEN_ID_OFFSET));
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
                    // <token_id(32)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
                    // <no_of_total_instances(4)><no_of_active_instances(4)><last_heartbeat_ledger(8)>
                    uint8_t host_addr[HOST_ADDR_VAL_SIZE];
                    if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not get host address state."), 1);

                    INT64_TO_BUF(&host_addr[HOST_HEARTBEAT_LEDGER_IDX_OFFSET], cur_ledger_seq);

                    if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not set state for heartbeat."), 1);

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
                    // <token_id(32)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
                    // <no_of_total_instances(4)><no_of_active_instances(4)><last_heartbeat_ledger(8)>
                    uint8_t host_addr[HOST_ADDR_VAL_SIZE];
                    if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not get host address state."), 1);

                    uint32_t section_number = 0, active_instances_len = 0;
                    uint8_t *active_instances_ptr;
                    int info_updated = 0;
                    for (int i = 0; GUARD(data_len), i < data_len; ++i)
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
                    }
                    // All data fields are optional in update info transaction. Update state only if an information update is detected.
                    if (info_updated)
                    {
                        uint32_t active_instances;
                        STR_TO_UINT(active_instances, active_instances_ptr, active_instances_len);

                        TRACEVAR(active_instances);

                        // Populate the values to the buffer.
                        UINT32_TO_BUF(host_addr + HOST_ACT_INS_COUNT_OFFSET, active_instances);

                        if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                            rollback(SBUF("Evernode: Could not set state for info update."), 1);
                    }

                    accept(SBUF("Evernode: Update host info successful."), 0);
                }

                accept(SBUF("Evernode: XRP transaction."), 0);
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
                    // <token_id(32)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
                    // <no_of_total_instances(4)><no_of_active_instances(4)><last_heartbeat_ledger(8)>
                    uint8_t host_addr[HOST_ADDR_VAL_SIZE];

                    if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) != DOESNT_EXIST)
                        rollback(SBUF("Evernode: Host already registered."), 1);

                    // Generate the NFT token id.

                    // Take the account secret from keylet.
                    uint8_t keylet[34];
                    if (util_keylet(SBUF(keylet), KEYLET_ACCOUNT, SBUF(hook_accid), 0, 0, 0, 0) != 34)
                        rollback(SBUF("Evernode: Could not generate the keylet for KEYLET_ACCOUNT."), 10);

                    int64_t slot_no = slot_set(SBUF(keylet), 0);
                    if (slot_no < 0)
                        rollback(SBUF("Evernode: Could not set keylet in slot"), 10);

                    int64_t token_seq_slot = slot_subfield(slot_no, sfMintedTokens, 0);
                    if (token_seq_slot < 0)
                        rollback(SBUF("Evernode: Could not find sfMintedTokens on hook account"), 20);

                    uint8_t token_seq_buf[4];
                    token_seq_slot = slot(SBUF(token_seq_buf), token_seq_slot);
                    uint32_t token_seq = UINT32_FROM_BUF(token_seq_buf);
                    TRACEVAR(token_seq);

                    // Transfer fee and Taxon will be 0 for the minted NFT.
                    uint32_t taxon = 0;
                    uint16_t tffee = 0;

                    uint8_t nft_token_id[NFT_TOKEN_ID_SIZE];
                    GENERATE_NFT_TOKEN_ID(nft_token_id, tffee, hook_accid, taxon, token_seq);
                    trace(SBUF("NFT token id:"), SBUF(nft_token_id), 1);

                    TOKEN_ID_KEY(nft_token_id);
                    if (state_set(SBUF(account_field), SBUF(STP_TOKEN_ID)) < 0)
                        rollback(SBUF("Evernode: Could not set state for token_id."), 1);

                    CLEARBUF(host_addr);
                    COPY_BUF(host_addr, 0, nft_token_id, 0, NFT_TOKEN_ID_SIZE);
                    COPY_BUF(host_addr, HOST_COUNTRY_CODE_OFFSET, data_ptr, 0, COUNTRY_CODE_LEN);

                    // Read instace details from the memo.
                    // We cannot predict the lengths of the numarical values.
                    // So we scan bytes and keep pointers and lengths to set in host address buffer.
                    uint32_t section_number = 0;
                    uint8_t *cpu_microsec_ptr, *ram_mb_ptr, *disk_mb_ptr, *total_ins_count_ptr, *description_ptr;
                    uint32_t cpu_microsec_len = 0, ram_mb_len = 0, disk_mb_len = 0, total_ins_count_len = 0, description_len = 0;

                    for (int i = 2; GUARD(data_len - 2), i < data_len; ++i)
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
                            if (description_len == 0)
                                description_ptr = str_ptr;
                            description_len++;
                        }
                    }

                    // Take the unsigned int values.
                    uint32_t cpu_microsec, ram_mb, disk_mb, total_ins_count;
                    STR_TO_UINT(cpu_microsec, cpu_microsec_ptr, cpu_microsec_len);
                    STR_TO_UINT(ram_mb, ram_mb_ptr, ram_mb_len);
                    STR_TO_UINT(disk_mb, disk_mb_ptr, disk_mb_len);
                    STR_TO_UINT(total_ins_count, total_ins_count_ptr, total_ins_count_len);

                    // Populate the values to the buffer.
                    UINT32_TO_BUF(&host_addr[HOST_CPU_MICROSEC_OFFSET], cpu_microsec);
                    UINT32_TO_BUF(&host_addr[HOST_RAM_MB_OFFSET], ram_mb);
                    UINT32_TO_BUF(&host_addr[HOST_DISK_MB_OFFSET], disk_mb);
                    CLEAR_BUF(host_addr, HOST_RESERVED_OFFSET, 8);
                    COPY_BUF(host_addr, HOST_DESCRIPTION_OFFSET, description_ptr, 0, description_len);
                    if (description_len < DESCRIPTION_LEN)
                        CLEAR_BUF(host_addr, HOST_DESCRIPTION_OFFSET + description_len, DESCRIPTION_LEN - description_len);
                    INT64_TO_BUF(&host_addr[HOST_REG_LEDGER_OFFSET], cur_ledger_seq);
                    UINT64_TO_BUF(&host_addr[HOST_REG_FEE_OFFSET], host_reg_fee);
                    UINT32_TO_BUF(&host_addr[HOST_TOT_INS_COUNT_OFFSET], total_ins_count);

                    if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not set state for host_addr."), 1);

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

                    int max_reached = 0;
                    if (host_reg_fee > conf_fixed_reg_fee && host_count > (conf_max_reg / 2))
                    {
                        max_reached = 1;
                        etxn_reserve(host_count + 3);
                    }
                    else
                        etxn_reserve(3);

                    // Froward 5 EVRs to foundation.
                    uint8_t foundation_accid[20];
                    if (state(SBUF(foundation_accid), SBUF(CONF_FOUNDATION_ADDR)) < 0)
                        rollback(SBUF("Evernode: Could not get foundation account address state."), 1);

                    uint8_t amt_out[AMOUNT_BUF_SIZE];
                    SET_AMOUNT_OUT(amt_out, EVR_TOKEN, issuer_accid, float_set(0, conf_fixed_reg_fee));
                    int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                    // Create the outgoing hosting token txn.
                    uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                    PREPARE_PAYMENT_SIMPLE_TRUSTLINE(txn_out, amt_out, fee, foundation_accid, 0, 0);

                    uint8_t emithash[32];
                    if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting EVR forward txn failed"), 1);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);

                    // Mint the nft token.
                    // Transaction URI would be the 'evrhost' + registration transaction hash.
                    uint8_t uri[39];
                    COPY_BUF_GUARDM(uri, 0, EVR_HOST, 0, 7, 1, 1);
                    COPY_BUF_GUARDM(uri, 7, txid, 0, HASH_SIZE, 1, 2);
                    fee = etxn_fee_base(PREPARE_NFT_MINT_SIZE(sizeof(uri)));

                    uint8_t nft_txn_out[PREPARE_NFT_MINT_SIZE(sizeof(uri))];
                    PREPARE_NFT_MINT(nft_txn_out, fee, tffee, taxon, uri, sizeof(uri));

                    if (emit(SBUF(emithash), SBUF(nft_txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting NFT mint txn failed"), 1);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);

                    // Create sell offer for the nft token.
                    fee = etxn_fee_base(PREPARE_NFT_SELL_OFFER_SIZE);

                    // Amount will be 0.
                    uint8_t offer_txn_out[PREPARE_NFT_SELL_OFFER_SIZE];
                    PREPARE_NFT_SELL_OFFER(offer_txn_out, 0, fee, account_field, nft_token_id);

                    if (emit(SBUF(emithash), SBUF(offer_txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting offer txn failed"), 1);
                    trace(SBUF("emit hash: "), SBUF(emithash), 1);

                    if (max_reached)
                    {
                        uint8_t state_buf[8] = {0};

                        host_reg_fee /= 2;
                        UINT64_TO_BUF(state_buf, host_reg_fee);
                        if (state_set(SBUF(state_buf), SBUF(STK_HOST_REG_FEE)) < 0)
                            rollback(SBUF("Evernode: Could not update the state for host reg fee."), 1);

                        conf_max_reg *= 2;
                        UINT64_TO_BUF(state_buf, conf_max_reg);
                        if (state_set(SBUF(state_buf), SBUF(STK_MAX_REG)) < 0)
                            rollback(SBUF("Evernode: Could not update state for max theoritical registrants."), 1);

                        // Refund the EVR balance.
                        const int outer_guard = token_seq + 1;
                        for (int i = 0; GUARD(outer_guard), i < outer_guard; ++i)
                        {
                            // Loop through all the possible token sequences and generate the token ids.
                            uint8_t lookup_id[NFT_TOKEN_ID_SIZE];
                            GENERATE_NFT_TOKEN_ID_GUARD(lookup_id, tffee, hook_accid, taxon, i, outer_guard);

                            // If the token id exists in the state (host is still registered),
                            // Rebate the halved registration fee.
                            TOKEN_ID_KEY_GUARD(lookup_id, outer_guard);
                            uint8_t host_accid[20] = {0};
                            if (state(SBUF(host_accid), SBUF(STP_TOKEN_ID)) != DOESNT_EXIST)
                            {
                                uint8_t amt_out[AMOUNT_BUF_SIZE];
                                SET_AMOUNT_OUT_GUARD(amt_out, EVR_TOKEN, issuer_accid, float_set(0, host_reg_fee), host_count);
                                fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                                // Create the outgoing hosting token txn.
                                uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                                PREPARE_PAYMENT_SIMPLE_TRUSTLINE_GUARD(txn_out, amt_out, fee, host_accid, host_count);

                                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                                    rollback(SBUF("Evernode: Emitting txn failed"), 1);
                                trace(SBUF("emit hash: "), SBUF(emithash), 1);

                                // Updating the current reg fee in the host state.
                                HOST_ADDR_KEY_GUARD(host_accid, host_count);
                                // <token_id(32)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
                                // <no_of_total_instances(4)><no_of_active_instances(4)><last_heartbeat_ledger(8)>
                                uint8_t rebate_host_addr[HOST_ADDR_VAL_SIZE];
                                if (state(SBUF(rebate_host_addr), SBUF(STP_HOST_ADDR)) < 0)
                                    rollback(SBUF("Evernode: Could not get host address state."), 1);
                                UINT64_TO_BUF(&rebate_host_addr[HOST_REG_FEE_OFFSET], host_reg_fee);
                                if (state_set(SBUF(rebate_host_addr), SBUF(STP_HOST_ADDR)) < 0)
                                    rollback(SBUF("Evernode: Could not update host address state."), 1);
                            }
                        }
                    }

                    accept(SBUF("Host registration successful."), 0);
                }
            }
        }
        else if (txn_type == ttNFT_ACCEPT_OFFER)
        {
            // Host deregistration nft accept.
            int is_host_de_reg_nft_accept = 0;
            BUFFER_EQUAL_STR(is_host_de_reg_nft_accept, type_ptr, type_len, HOST_POST_DEREG);
            if (is_host_de_reg_nft_accept)
            {
                BUFFER_EQUAL_STR_GUARD(is_host_de_reg_nft_accept, format_ptr, format_len, FORMAT_HEX, 1);
                if (!is_host_de_reg_nft_accept)
                    rollback(SBUF("Evernode: Memo format should be hex."), 1);

                // Check whether the host address state is deleted.
                HOST_ADDR_KEY(account_field);
                uint8_t host_addr[HOST_ADDR_VAL_SIZE];
                if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) != DOESNT_EXIST)
                    rollback(SBUF("Evernode: The host address state exists."), 1);

                // Check the ownership of the NFT to the hook before proceeding.
                uint16_t flags;
                uint8_t issuer[20], uri[256], uri_len;
                uint32_t taxon;
                if (get_nft(hook_accid, data_ptr, &flags, issuer, &taxon, uri, &uri_len) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: Token mismatch with registration."), 1);

                // Check whether the NFT URI is starting with 'evrhost'.
                BUFFER_EQUAL_STR_GUARD(is_host_de_reg_nft_accept, uri, 7, EVR_HOST, 1);
                if (!is_host_de_reg_nft_accept)
                    rollback(SBUF("Evernode: Provided NFT is invalid."), 1);

                // Check whether the host token id state is deleted.
                TOKEN_ID_KEY(data_ptr);
                uint8_t token_id[TOKEN_ID_VAL_SIZE];
                if (state(SBUF(token_id), SBUF(STP_TOKEN_ID)) != DOESNT_EXIST)
                    rollback(SBUF("Evernode: The host token id state exists."), 1);

                // Burn the NFT.
                etxn_reserve(1);

                int64_t fee = etxn_fee_base(PREPARE_NFT_BURN_SIZE);
                uint8_t txn_out[PREPARE_NFT_BURN_SIZE];
                PREPARE_NFT_BURN(txn_out, fee, data_ptr, hook_accid);

                uint8_t emithash[HASH_SIZE];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting NFT burn txn failed"), 1);
                trace(SBUF("emit hash: "), SBUF(emithash), 1);

                accept(SBUF("Host de-registration nft accept successful."), 0);
            }
        }
    }

    accept(SBUF("Evernode: Transaction type not supported."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
