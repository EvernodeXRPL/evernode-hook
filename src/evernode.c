#include "../lib/hookapi.h"
// #include "../lib/emulatorapi.h"
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

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    uint8_t keylet[34];
    if (util_keylet(SBUF(keylet), KEYLET_ACCOUNT, SBUF(hook_accid), 0, 0, 0, 0) != 34)
        rollback(SBUF("Evernode: Could not generate the keylet for KEYLET_ACCOUNT."), 10);

    int64_t slot_no = slot_set(SBUF(keylet), 0);
    if (slot_no < 0)
        rollback(SBUF("Evernode: Could not set keylet in slot"), 10);

    int64_t seq_slot = slot_subfield(slot_no, sfSequence, 0);
    if (seq_slot < 0)
        rollback(SBUF("Evernode: Could not find sfSequence on hook account"), 20);

    uint8_t sequence_buf[4];
    seq_slot = slot(SBUF(sequence_buf), seq_slot);
    uint32_t sequence = UINT32_FROM_BUF(sequence_buf);
    TRACEVAR(sequence);

    /////////////////////////////////////// Tests ///////////////////////////////////////

    etxn_reserve(2);

    uint8_t uri[20] = "this is test uri abc";
    int64_t fee = etxn_fee_base(PREPARE_NFT_MINT_SIZE(sizeof(uri)));
    unsigned char tx[PREPARE_NFT_MINT_SIZE(sizeof(uri))];

    PREPARE_NFT_MINT(tx, fee, 10, 2, uri, sizeof(uri));

    trace(SBUF("Transaction: "), SBUF(tx), 1);

    uint8_t emithash[32];
    if (emit(SBUF(emithash), SBUF(tx)) < 0)
        rollback(SBUF("Evernode: Emitting txn failed"), 1);
    trace(SBUF("emit hash: "), SBUF(emithash), 1);

    fee = etxn_fee_base(PREPARE_NFT_SELL_OFFER_SIZE);
    unsigned char tx2[PREPARE_NFT_SELL_OFFER_SIZE];

    uint8_t accid[20];
    const int accid_len = util_accid(SBUF(accid), SBUF("rKtwPkSUNQ3X9vsZLqNcYUGwa7vJ5bG7kA"));
    trace(SBUF("ACCID: "), SBUF(accid), 1);

    uint8_t tknid[32] = {0, 8, 0, 0, 162, 41, 141, 52, 207, 250, 179, 111, 54, 174, 245, 55, 40, 82, 183, 39, 216, 197, 232, 2, 193, 180, 83, 197, 0, 0, 0, 42};
    PREPARE_NFT_SELL_OFFER(tx2, 1, fee, accid, tknid);

    trace(SBUF("Transaction: "), SBUF(tx2), 1);

    if (emit(SBUF(emithash), SBUF(tx2)) < 0)
        rollback(SBUF("Evernode: Emitting txn failed"), 1);
    trace(SBUF("emit hash: "), SBUF(emithash), 1);

    accept(SBUF("Host registration successful."), 0);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    int64_t txn_type = otxn_type();
    if (txn_type == ttPAYMENT)
    {
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

        // Memos
        uint8_t memos[MAX_MEMO_SIZE];
        int64_t memos_len = otxn_field(SBUF(memos), sfMemos);

        if (!memos_len)
            accept(SBUF("Evernode: No memos found."), 1);

        uint8_t *memo_ptr, *type_ptr, *format_ptr, *data_ptr;
        uint32_t memo_len, type_len, format_len, data_len;
        GET_MEMO(0, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len);

        if (is_xrp)
        {
            // Host initialization.
            int is_initialize = 0;
            BUFFER_EQUAL_STR_GUARD(is_initialize, type_ptr, type_len, INITIALIZE, 1);
            if (is_initialize)
            {
                uint8_t foundation_accid[20];
                const int foundation_accid_len = util_accid(SBUF(foundation_accid), FOUNDATION_ADDR, 35);
                if (foundation_accid_len < 20)
                    rollback(SBUF("Evernode: Could not convert foundation account id."), 1);

                int is_foundation = 0;
                BUFFER_EQUAL(is_foundation, foundation_accid, account_field, 20);
                if (!is_foundation)
                    rollback(SBUF("Evernode: Only foundation is allowed to initialize state."), 1);

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

                uint8_t issuer_accid[20];
                const int issuer_accid_len = util_accid(SBUF(issuer_accid), ISSUER_ADDR, 35);
                if (issuer_accid_len < 20)
                    rollback(SBUF("Evernode: Could not convert issuer account id."), 1);

                if (state_set(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR)) < 0)
                    rollback(SBUF("Evernode: Could not set state for issuer account."), 1);

                if (state_set(SBUF(foundation_accid), SBUF(CONF_FOUNDATION_ADDR)) < 0)
                    rollback(SBUF("Evernode: Could not set state for foundation account."), 1);

                SET_UINT_STATE_VALUE(DEF_MOMENT_SIZE, CONF_MOMENT_SIZE, "Evernode: Could not initialize state for moment size.");
                SET_UINT_STATE_VALUE(DEF_MINT_LIMIT, CONF_MINT_LIMIT, "Evernode: Could not initialize state for mint limit.");
                SET_UINT_STATE_VALUE(DEF_FIXED_REG_FEE, CONF_FIXED_REG_FEE, "Evernode: Could not initialize state for fixed reg fee.");
                SET_UINT_STATE_VALUE(DEF_HOST_HEARTBEAT_FREQ, CONF_HOST_HEARTBEAT_FREQ, "Evernode: Could not initialize state for heartbeat frequency.");

                accept(SBUF("Evernode: Initialization successful."), 0);
            }

            // Host deregistration.
            int is_host_de_reg = 0;
            BUFFER_EQUAL_STR_GUARD(is_host_de_reg, type_ptr, type_len, HOST_DE_REG, 1);
            if (is_host_de_reg)
            {
                accept(SBUF("Evernode: Host de-registration successful."), 0);
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

            int is_evr;
            IS_EVR(is_evr, amount_buffer, hook_accid);

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
                    rollback(SBUF("Evernode: Memo format should be text."), 50);

                // Take the host reg fee from config.
                int64_t host_reg_fee;
                GET_CONF_VALUE(host_reg_fee, STK_HOST_REG_FEE, "Evernode: Could not get host reg fee state.");
                TRACEVAR(host_reg_fee);

                if (float_compare(amt, host_reg_fee, COMPARE_LESS) == 1)
                    rollback(SBUF("Evernode: Amount sent is less than the minimum fee for host registration."), 1);

                // Checking whether this host is already registered.
                HOST_ADDR_KEY(account_field);
                // <token_id(32)><hosting_token(3)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><registration_ledger(8)><registration_fee(8)>
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

                int64_t seq_slot = slot_subfield(slot_no, sfSequence, 0);
                if (seq_slot < 0)
                    rollback(SBUF("Evernode: Could not find sfSequence on hook account"), 20);

                uint8_t sequence_buf[4];
                seq_slot = slot(SBUF(sequence_buf), seq_slot);
                uint32_t sequence = UINT32_FROM_BUF(sequence_buf);
                TRACEVAR(sequence);

                uint8_t nft_tkn_id[NFT_TOKEN_ID_SIZE];

                COPY_BUF(nft_tkn_id, 0, TOKEN_ID_PREFIX, 0, 4);
                COPY_BUF(nft_tkn_id, 4, hook_accid, 0, 20);
                CLEAR_BUF(nft_tkn_id, 24, 4);
                UINT32_TO_BUF(nft_tkn_id + 28, sequence);
                trace(SBUF("NFT token id:"), SBUF(nft_tkn_id), 1);

                TOKEN_ID_KEY(nft_tkn_id);
                if (state_set(SBUF(account_field), SBUF(STP_TOKEN_ID)) < 0)
                    rollback(SBUF("Evernode: Could not set state for token_id."), 1);

                CLEARBUF(host_addr);
                COPY_BUF(host_addr, 0, nft_tkn_id, 0, 4);
                COPY_BUF(host_addr, HOST_TOKEN_OFFSET, data_ptr, 0, 3);
                COPY_BUF(host_addr, HOST_COUNTRY_CODE_OFFSET, data_ptr, 4, COUNTRY_CODE_LEN);

                // Read instace details from the memo.
                // We cannot predict the lengths of the numarical values.
                // So we scan bytes and keep pointers and lengths to set in host address buffer.
                uint32_t section_number = 0;
                uint8_t *cpu_microsec_ptr, *ram_mb_ptr, *disk_mb_ptr, *description_ptr;
                uint32_t cpu_microsec_len = 0, ram_mb_len = 0, disk_mb_len = 0, description_len = 0;

                for (int i = 6; GUARD(data_len - 6), i < data_len; ++i)
                {
                    uint8_t *str_ptr = data_ptr + i;
                    // Colon means this is an end of the section.
                    // If so, we start reading the new section and reset the write index.
                    // Stop reading is an emty byte reached.
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
                        if (description_len == 0)
                            description_ptr = str_ptr;
                        description_len++;
                    }
                }

                // Take the unsigned int values.
                uint32_t cpu_microsec, ram_mb, disk_mb;
                STR_TO_UINT(cpu_microsec, cpu_microsec_ptr, cpu_microsec_len);
                STR_TO_UINT(ram_mb, ram_mb_ptr, ram_mb_len);
                STR_TO_UINT(disk_mb, disk_mb_ptr, disk_mb_len);

                // Populate the values to the buffer.
                UINT32_TO_BUF(&host_addr[HOST_CPU_MICROSEC_OFFSET], cpu_microsec);
                UINT32_TO_BUF(&host_addr[HOST_RAM_MB_OFFSET], ram_mb);
                UINT32_TO_BUF(&host_addr[HOST_DISK_MB_OFFSET], disk_mb);
                CLEAR_BUF(host_addr, HOST_RESERVED_OFFSET, 8);
                COPY_BUF(host_addr, HOST_DESCRIPTION_OFFSET, description_ptr, 0, description_len);
                if (description_len < DESCRIPTION_LEN)
                    CLEAR_BUF(host_addr, HOST_DESCRIPTION_OFFSET + description_len, DESCRIPTION_LEN - description_len);
                INT64_TO_BUF(&host_addr[HOST_REG_LEDGER_OFFSET], cur_ledger_seq);
                INT64_TO_BUF(&host_addr[HOST_REG_FEE_OFFSET], host_reg_fee);

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

                uint8_t issuer_accid[20];
                if (state(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR)) != DOESNT_EXIST)
                    rollback(SBUF("Evernode: Could not get issuer address state."), 1);

                int max_reached = 0;
                if (float_compare(conf_fixed_reg_fee, host_reg_fee, COMPARE_EQUAL) != 1 && host_count > (conf_max_reg * 50 / 100))
                {
                    max_reached = 1;
                    etxn_reserve(host_count + 2);
                }
                else
                    etxn_reserve(2);

                // Froward 5 EVRs to foundation.
                uint8_t foundation_accid[20];
                if (state(SBUF(foundation_accid), SBUF(CONF_FOUNDATION_ADDR)) != DOESNT_EXIST)
                    rollback(SBUF("Evernode: Could not get foundation account address state."), 1);

                uint8_t amt_out[AMOUNT_BUF_SIZE];
                SET_AMOUNT_OUT(amt_out, EVR_TOKEN, issuer_accid, conf_fixed_reg_fee);
                int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                // Create the outgoing hosting token txn.
                uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                PREPARE_PAYMENT_SIMPLE_TRUSTLINE(txn_out, amt_out, fee, foundation_accid, 0, 0);

                // Mint the nft token.
                // Transaction URI would be the registration transaction hash.
                fee = etxn_fee_base(PREPARE_NFT_MINT_SIZE(sizeof(txid)));
                unsigned char tx[PREPARE_NFT_MINT_SIZE(sizeof(txid))];

                // Transfer fee and Taxon will be 0.
                PREPARE_NFT_MINT(tx, fee, 0, 0, txid, sizeof(txid));

                uint8_t emithash[32];
                if (emit(SBUF(emithash), SBUF(tx)) < 0)
                    rollback(SBUF("Evernode: Emitting txn failed"), 1);
                trace(SBUF("emit hash: "), SBUF(emithash), 1);

                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting txn failed"), 1);
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
                    for (int i = 1; GUARD(host_count), i <= host_count; ++i)
                    {
                        uint8_t host_accid[20] = {0};

                        uint8_t amt_out[AMOUNT_BUF_SIZE];
                        SET_AMOUNT_OUT(amt_out, EVR_TOKEN, issuer_accid, host_reg_fee);
                        fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                        // Create the outgoing hosting token txn.
                        uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                        PREPARE_PAYMENT_SIMPLE_TRUSTLINE(txn_out, amt_out, fee, host_accid, 0, 0);

                        if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                            rollback(SBUF("Evernode: Emitting txn failed"), 1);
                        trace(SBUF("emit hash: "), SBUF(emithash), 1);
                    }
                }

                accept(SBUF("Host registration successful."), 0);
            }
        }
    }

    accept(SBUF("Evernode: Transaction type not supported."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
