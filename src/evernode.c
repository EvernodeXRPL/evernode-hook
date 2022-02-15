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
            // Host deregistration.
            int is_host_de_reg = 0;
            BUFFER_EQUAL_STR_GUARD(is_host_de_reg, type_ptr, type_len, HOST_DE_REG, 1);
            if (is_host_de_reg)
            {
                // Host de register is only served if at least one host is registered.
                uint8_t host_count_buf[4];
                uint32_t host_count;
                GET_HOST_COUNT(host_count_buf, host_count);
                if (host_count == 0)
                    rollback(SBUF("Evernode: No hosts registered to de register."), 1);

                HOST_ADDR_KEY(account_field);
                // Check whether the host is registered.
                // <host_id(4)><hosting_token(3)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><audit_assigned_moment_start_idx(8)>
                // <auditor_addr(20)><rewarded_moment_start_idx(8)><accumulated_rewards(8)><locked_token_amont(8)><last_hearbeat_ledger_idx(8)>
                uint8_t host_addr_data[HOST_ADDR_VAL_SIZE];
                if (state(SBUF(host_addr_data), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: This host is not registered."), 1);

                uint8_t *host_token_ptr = &host_addr_data[HOST_TOKEN_OFFSET];

                // Reserving two transaction.
                etxn_reserve(2);

                uint8_t hosting_token[20] = GET_TOKEN_CURRENCY(host_token_ptr);
                uint8_t keylet[34];
                if (util_keylet(SBUF(keylet), KEYLET_LINE, SBUF(hook_accid), SBUF(account_field), SBUF(hosting_token)) != 34)
                    rollback(SBUF("Evernode: Internal error, could not generate keylet for host deregistration"), 1);

                int64_t hook_host_trustline_slot = slot_set(SBUF(keylet), 0);
                if (hook_host_trustline_slot < 0)
                    rollback(SBUF("Evernode: Hook must have a trustline set for HTK to this account."), 1);

                int compare_result = 0;
                ACCOUNT_COMPARE(compare_result, hook_accid, account_field);
                if (compare_result == 0)
                    rollback(SBUF("Evernode: Invalid trustline set hi=lo?"), 1);

                int64_t balance_slot = slot_subfield(hook_host_trustline_slot, sfBalance, 0);
                if (balance_slot < 0)
                    rollback(SBUF("Evernode: Could not find sfBalance on trustline"), 1);


                int64_t balance_float = slot_float(balance_slot);

                // Hook gets a negative trustline balance since hook owes tokens to the host. 
                if (float_compare(balance_float, 0, COMPARE_LESS) == 1)
                    balance_float = float_negate(balance_float);

                uint8_t amt_out[AMOUNT_BUF_SIZE];
                // If value is < 0 or not 0 value is invalid.
                if (balance_float < 0 && !IS_FLOAT_ZERO(balance_float))
                    rollback(SBUF("Evernode: Could not parse user trustline balance."), 1);
                else if (!IS_FLOAT_ZERO(balance_float)) // If we have positive balance.
                {
                    // Return the available balance to the host before de-registering.

                    // Setup the outgoing txn.
                    int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                    uint8_t amt_out[AMOUNT_BUF_SIZE];
                    SET_AMOUNT_OUT(amt_out, host_token_ptr, account_field, balance_float);

                    // Finally create the outgoing txn.
                    uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                    PREPARE_PAYMENT_SIMPLE_TRUSTLINE(txn_out, amt_out, fee, account_field, 0, 0);

                    uint8_t emithash[HASH_SIZE];
                    if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                        rollback(SBUF("Evernode: Emitting remaining token transaction failed."), 1);

                    trace(SBUF("returned tx hash: "), SBUF(emithash), 1);
                }

                // Clear amt_out buffer before re-using it.
                CLEARBUF(amt_out);

                SET_AMOUNT_OUT(amt_out, host_token_ptr, account_field, 0);

                int64_t fee = etxn_fee_base(PREPARE_SIMPLE_TRUSTLINE_SIZE);

                // Preparing trustline transaction.
                uint8_t txn_out[PREPARE_SIMPLE_TRUSTLINE_SIZE];
                PREPARE_SIMPLE_TRUSTLINE(txn_out, amt_out, fee);

                uint8_t emithash[HASH_SIZE];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting txn failed in token 0 trustline."), 1);

                trace(SBUF("emit hash token 0: "), SBUF(emithash), 1);

                // Cleanup the state objects.
                if (host_count > 1)
                {
                    uint8_t last_host_addr[20];
                    uint8_t last_host_id_buf[HOST_ADDR_VAL_SIZE];
                    // Get the address of the host with last host id which is equal to the host count.
                    HOST_ID_KEY(host_count_buf);
                    if (state(SBUF(last_host_addr), SBUF(STP_HOST_ID)) != 20)
                        rollback(SBUF("Evernode: Could not get last host address."), 1);

                    HOST_ADDR_KEY(last_host_addr);
                    if (state(SBUF(last_host_id_buf), SBUF(STP_HOST_ADDR)) != HOST_ADDR_VAL_SIZE)
                        rollback(SBUF("Evernode: Could not get last host id data."), 1);

                    // Update the last host entry with the deleting host id.
                    COPY_BUF(last_host_id_buf, 0, host_addr_data, 0, 4);

                    HOST_ID_KEY(host_addr_data);
                    if (state_set(SBUF(last_host_id_buf), SBUF(STP_HOST_ADDR)) < 0 || state_set(SBUF(last_host_addr), SBUF(STP_HOST_ID)) < 0)
                        rollback(SBUF("Evernode: Could not update state for host_id."), 1);
                }

                // Delete entries.
                HOST_ADDR_KEY(account_field);
                HOST_ID_KEY(host_count_buf);
                if (state_set(0, 0, SBUF(STP_HOST_ADDR)) < 0 || state_set(0, 0, SBUF(STP_HOST_ID)) < 0)
                    rollback(SBUF("Evernode: Could not delete host entries in host de-registration."), 1);

                host_count -= 1;
                UINT32_TO_BUF(host_count_buf, host_count);
                if (state_set(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) < 0)
                    rollback(SBUF("Evernode: Could not reduce host count in host de-registration."), 1);

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

            // Get transaction hash(id).
            uint8_t txid[HASH_SIZE];
            int32_t txid_len = otxn_id(SBUF(txid), 0);
            if (txid_len < HASH_SIZE)
                rollback(SBUF("Evernode: transaction id missing!!!"), 1);

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
                int64_t conf_host_reg_fee;
                GET_FLOAT_CONF_VALUE(conf_host_reg_fee, DEF_HOST_REG_FEE_M, DEF_HOST_REG_FEE_E, CONF_HOST_REG_FEE, "Evernode: Could not set default state for host reg fee.");
                TRACEVAR(conf_host_reg_fee);

                if (float_compare(amt, conf_host_reg_fee, COMPARE_LESS) == 1)
                    rollback(SBUF("Evernode: Amount sent is less than the minimum fee for host registration."), 1);

                // Checking whether this host is already registered.
                HOST_ADDR_KEY(account_field);
                // <host_id(4)><hosting_token(3)><country_code(2)><cpu_microsec(4)><ram_mb(4)><disk_mb(4)><reserved(8)><description(26)><audit_assigned_moment_start_idx(8)>
                // <auditor_addr(20)><rewarded_moment_start_idx(8)><accumulated_rewards(8)><locked_token_amont(8)><last_hearbeat_ledger_idx(8)>
                uint8_t host_addr[HOST_ADDR_VAL_SIZE];

                if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) != DOESNT_EXIST)
                    rollback(SBUF("Evernode: Host already registered."), 1);

                // Generate transaction with following properties.
                /**
                    Transaction type: Trust Set
                    Source: Hook
                    Destination: Host
                    Currency: hosting_token
                    Limit: 999999999
                    */

                // Reserving one transaction.
                etxn_reserve(1);
                // Calculate fee for trustline transaction.
                int64_t fee = etxn_fee_base(PREPARE_SIMPLE_TRUSTLINE_SIZE);

                int64_t token_limit = float_set(0, 999999999); // 999999999

                uint8_t amt_out[AMOUNT_BUF_SIZE];
                SET_AMOUNT_OUT(amt_out, data_ptr, account_field, token_limit);

                // Preparing trustline transaction.
                uint8_t txn_out[PREPARE_SIMPLE_TRUSTLINE_SIZE];
                PREPARE_SIMPLE_TRUSTLINE(txn_out, amt_out, fee);

                uint8_t emithash[HASH_SIZE];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting txn failed"), 1);

                trace(SBUF("emit hash: "), SBUF(emithash), 1);

                uint8_t host_count_buf[4];
                uint32_t host_count;
                GET_HOST_COUNT(host_count_buf, host_count);

                uint32_t host_id = host_count + 1;
                uint8_t host_id_arr[4];
                UINT32_TO_BUF(host_id_arr, host_id);
                HOST_ID_KEY(host_id_arr);
                if (state_set(SBUF(account_field), SBUF(STP_HOST_ID)) < 0)
                    rollback(SBUF("Evernode: Could not set state for host_id."), 1);

                CLEARBUF(host_addr);
                COPY_BUF(host_addr, 0, host_id_arr, 0, 4);
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

                if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                    rollback(SBUF("Evernode: Could not set state for host_addr."), 1);

                host_count += 1;
                UINT32_TO_BUF(host_count_buf, host_count);
                if (state_set(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) < 0)
                    rollback(SBUF("Evernode: Could not set default state for host count."), 1);

                accept(SBUF("Host registration successful."), 0);
            }
        }
    }

    accept(SBUF("Evernode: Transaction type not supported."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
