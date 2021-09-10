#include "../lib/hookapi.h"
#include "evernode.h"

#define MAX_MEMO_SIZE 4096 // Maximum tx blob size.

// Executed when an emitted transaction is successfully accepted into a ledger
// or when an emitted transaction cannot be accepted into any ledger (with what = 1),
int64_t cbak(int64_t reserved)
{
    return 0;
}

// Executed whenever a transaction comes into or leaves from the account the Hook is set on.
int64_t hook(int64_t reserved)
{
    // *************************************************************************************
    // ***********************Config variable managment code********************************
    // *************************************************************************************

    uint8_t host_count_buf[4] = {0};
    if (state(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) == DOESNT_EXIST)
    {
        if (state_set(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) < 0)
            rollback(SBUF("Evernode: Could not set default state for host count."), 1);
    }
    uint32_t host_count = UINT32_FROM_BUF(host_count_buf);
    TRACEVAR(host_count);

    uint8_t auditor_count_buf[4] = {0};
    if (state(SBUF(auditor_count_buf), SBUF(STK_AUDITOR_COUNT)) == DOESNT_EXIST)
    {
        if (state_set(SBUF(auditor_count_buf), SBUF(STK_AUDITOR_COUNT)) < 0)
            rollback(SBUF("Evernode: Could not set default state for auditor count."), 1);
    }
    uint32_t auditor_count = UINT32_FROM_BUF(auditor_count_buf);
    TRACEVAR(auditor_count);

    // Setting and loading configuration values from the hook state.
    uint8_t conf_moment_size_buf[2];
    if (state(SBUF(conf_moment_size_buf), SBUF(CONF_MOMENT_SIZE)) == DOESNT_EXIST)
    {
        UINT16_TO_BUF(conf_moment_size_buf, DEF_MOMENT_SIZE);
        if (state_set(SBUF(conf_moment_size_buf), SBUF(CONF_MOMENT_SIZE)) < 0)
            rollback(SBUF("Evernode: Could not set default state for moment size."), 1);
    }
    uint16_t conf_moment_size = UINT16_FROM_BUF(conf_moment_size_buf);
    TRACEVAR(conf_moment_size);

    uint8_t conf_mint_limit_buf[8];
    if (state(SBUF(conf_mint_limit_buf), SBUF(CONF_MINT_LIMIT)) == DOESNT_EXIST)
    {
        UINT64_TO_BUF(conf_mint_limit_buf, DEF_MINT_LIMIT);
        if (state_set(SBUF(conf_mint_limit_buf), SBUF(CONF_MINT_LIMIT)) < 0)
            rollback(SBUF("Evernode: Could not set default state for mint limit."), 1);
    }
    uint64_t conf_mint_limit = UINT64_FROM_BUF(conf_mint_limit_buf);
    TRACEVAR(conf_mint_limit);

    uint8_t conf_host_reg_fee_buf[2];
    if (state(SBUF(conf_host_reg_fee_buf), SBUF(CONF_HOST_REG_FEE)) == DOESNT_EXIST)
    {
        UINT16_TO_BUF(conf_host_reg_fee_buf, DEF_HOST_REG_FEE);
        if (state_set(SBUF(conf_host_reg_fee_buf), SBUF(CONF_HOST_REG_FEE)) < 0)
            rollback(SBUF("Evernode: Could not set default state for host reg fee."), 1);
    }
    uint16_t conf_host_reg_fee = UINT16_FROM_BUF(conf_host_reg_fee_buf);
    TRACEVAR(conf_host_reg_fee);

    uint8_t conf_min_redeem_buf[2];
    if (state(SBUF(conf_min_redeem_buf), SBUF(CONF_MIN_REDEEM)) == DOESNT_EXIST)
    {
        UINT16_TO_BUF(conf_min_redeem_buf, DEF_MIN_REDEEM);
        if (state_set(SBUF(conf_min_redeem_buf), SBUF(CONF_MIN_REDEEM)) < 0)
            rollback(SBUF("Evernode: Could not set default state for min redeem."), 1);
    }
    uint16_t conf_min_redeem = UINT16_FROM_BUF(conf_min_redeem_buf);
    TRACEVAR(conf_min_redeem);

    uint8_t conf_redeem_window_buf[2];
    if (state(SBUF(conf_redeem_window_buf), SBUF(CONF_REDEEM_WINDOW)) == DOESNT_EXIST)
    {
        UINT16_TO_BUF(conf_redeem_window_buf, DEF_REDEEM_WINDOW);
        if (state_set(SBUF(conf_redeem_window_buf), SBUF(CONF_REDEEM_WINDOW)) < 0)
            rollback(SBUF("Evernode: Could not set default state for redeem window."), 1);
    }
    uint16_t conf_redeem_window = UINT16_FROM_BUF(conf_redeem_window_buf);
    TRACEVAR(conf_redeem_window);

    uint8_t conf_host_reward_buf[2];
    if (state(SBUF(conf_host_reward_buf), SBUF(CONF_HOST_REWARD)) == DOESNT_EXIST)
    {
        UINT16_TO_BUF(conf_host_reward_buf, DEF_HOST_REWARD);
        if (state_set(SBUF(conf_host_reward_buf), SBUF(CONF_HOST_REWARD)) < 0)
            rollback(SBUF("Evernode: Could not set default state for host reward."), 1);
    }
    uint16_t conf_host_reward = UINT16_FROM_BUF(conf_host_reward_buf);
    TRACEVAR(conf_host_reward);
    // ************************Config variable managment code end **************************

    // Getting the hook account id.
    unsigned char hook_accid[20];
    hook_account((uint32_t)hook_accid, 20);

    // Next fetch the sfAccount field from the originating transaction
    uint8_t account_field[20];
    uint8_t dest_field[20];
    int32_t account_field_len = otxn_field(SBUF(account_field), sfAccount);
    int32_t dest_field_len = otxn_field(SBUF(dest_field), sfDestination);
    if (account_field_len < 20 || dest_field_len < 20)
        rollback(SBUF("Evernode: sfAccount or sfDestination field missing!!!"), 10);

    // specifically we're interested in the amount sent
    int64_t oslot = otxn_slot(0);
    if (oslot < 0)
        rollback(SBUF("Evernode: Could not slot originating txn."), 1);

    int64_t amt_slot = slot_subfield(oslot, sfAmount, 0);

    if (amt_slot < 0)
        rollback(SBUF("Evernode: Could not slot otxn.sfAmount"), 2);

    int64_t is_xrp = slot_type(amt_slot, 1);
    if (is_xrp < 0)
        rollback(SBUF("Evernode: Could not determine sent amount type"), 3);

    int64_t txn_type = otxn_type();
    if (txn_type == ttPAYMENT)
    {
        // Memos
        uint8_t memos[MAX_MEMO_SIZE];
        int64_t memos_len = otxn_field(SBUF(memos), sfMemos);

        if (!memos_len)
            accept(SBUF("Evernode: No memos found."), 1);

        // since our memos are in a buffer inside the hook (as opposed to being a slot) we use the sto api with it
        // the sto apis probe into a serialized object returning offsets and lengths of subfields or array entries
        int64_t memo_lookup = sto_subarray(memos, memos_len, 0);

        uint8_t *memo_ptr = SUB_OFFSET(memo_lookup) + memos;
        uint32_t memo_len = SUB_LENGTH(memo_lookup);

        // memos are nested inside an actual memo object, so we need to subfield
        // equivalently in JSON this would look like memo_array[i]["Memo"]
        memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemo);
        memo_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;
        memo_len = SUB_LENGTH(memo_lookup);

        if (memo_lookup < 0)
            accept(SBUF("Evernode: Incoming txn had a blank sfMemos."), 1);

        int64_t type_lookup = sto_subfield(memo_ptr, memo_len, sfMemoType);
        uint8_t *type_ptr = SUB_OFFSET(type_lookup) + memo_ptr;
        uint32_t type_len = SUB_LENGTH(type_lookup);
        // trace(SBUF("type in hex: "), type_ptr, type_len, 1);

        int64_t format_lookup = sto_subfield(memo_ptr, memo_len, sfMemoFormat);
        uint8_t *format_ptr = SUB_OFFSET(format_lookup) + memo_ptr;
        uint32_t format_len = SUB_LENGTH(format_lookup);
        // trace(SBUF("format in hex: "), format_ptr, format_len, 1);

        int64_t data_lookup = sto_subfield(memo_ptr, memo_len, sfMemoData);
        uint8_t *data_ptr = SUB_OFFSET(data_lookup) + memo_ptr;
        uint32_t data_len = SUB_LENGTH(data_lookup);
        // trace(SBUF("data in hex: "), data_ptr, data_len, 1); // Text data is in hex format.

        if (is_xrp)
        {
            int is_redeem_ref = 0;
            BUFFER_EQUAL_STR_GUARD(is_redeem_ref, type_ptr, type_len, "evnRedeemRef", 1);
            if (is_redeem_ref)
            {
                int is_format_match = 0;
                BUFFER_EQUAL_STR_GUARD(is_format_match, format_ptr, format_len, "binary", 1);
                if (!is_format_match)
                    rollback(SBUF("Evernode: Memo format should be binary."), 50);

                if (data_len < 32)
                    rollback(SBUF("Evernode: Invalid redeem reference."), 1);

                // Redeem response has 2 memos, so check for the type in second memo.
                memo_lookup = sto_subarray(memos, memos_len, 1);

                memo_ptr = SUB_OFFSET(memo_lookup) + memos;
                memo_len = SUB_LENGTH(memo_lookup);

                memo_lookup = sto_subfield(memo_ptr, memo_len, sfMemo);
                memo_ptr = SUB_OFFSET(memo_lookup) + memo_ptr;
                memo_len = SUB_LENGTH(memo_lookup);

                if (memo_lookup < 0)
                    accept(SBUF("Evernode: Incoming txn had a blank sfMemos."), 1);

                type_lookup = sto_subfield(memo_ptr, memo_len, sfMemoType);
                type_ptr = SUB_OFFSET(type_lookup) + memo_ptr;
                type_len = SUB_LENGTH(type_lookup);
                // trace(SBUF("type in hex: "), type_ptr, type_len, 1);

                format_lookup = sto_subfield(memo_ptr, memo_len, sfMemoFormat);
                format_ptr = SUB_OFFSET(format_lookup) + memo_ptr;
                format_len = SUB_LENGTH(format_lookup);
                // trace(SBUF("format in hex: "), format_ptr, format_len, 1);

                int is_redeem_res = 0;
                BUFFER_EQUAL_STR_GUARD(is_redeem_res, type_ptr, type_len, "evnRedeemResp", 1);
                if (!is_redeem_res)
                    rollback(SBUF("Evernode: Redeem response does not have a evnPedeemResp."), 1);

                is_format_match = 0;
                BUFFER_EQUAL_STR_GUARD(is_format_match, format_ptr, format_len, "binary", 1);
                if (!is_format_match)
                    rollback(SBUF("Evernode: Memo format should be binary."), 50);

                REDEEM_OP_KEY(data_ptr);

                uint8_t redeem_op[39];
                if (state(SBUF(redeem_op), SBUF(STP_REDEEM_OP)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: No redeem state for the redeem response."), 1);

                // Reserving one transaction.
                etxn_reserve(1);

                // get the source tag if any... negative means it wasn't provided
                int64_t source_tag = otxn_field(0, 0, sfSourceTag);
                if (source_tag < 0)
                    source_tag = 0xFFFFFFFFU;

                // setup the outgoing txn
                int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

                uint8_t host_currency[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, redeem_op[0], redeem_op[1], redeem_op[2], 0, 0, 0, 0, 0};
                uint8_t host_amount_buf[8];
                for (int i = 0; GUARD(8), i < 8; ++i)
                    host_amount_buf[i] = redeem_op[i + 3];
                int64_t host_amount = UINT64_FROM_BUF(host_amount_buf);
                uint8_t host_issuer[20];
                for (int i = 0; GUARD(20), i < 20; ++i)
                    host_issuer[i] = redeem_op[i + 11];

                // we need to dump the iou amount into a buffer
                // by supplying -1 as the fieldcode we tell float_sto not to prefix an actual STO header on the field
                uint8_t amt_out[48];
                if (float_sto(SBUF(amt_out), SBUF(host_currency), SBUF(host_issuer), host_amount, -1) < 0)
                    rollback(SBUF("Evernode: Could not dump HTK amount into sto"), 1);

                // set the currency code and issuer in the amount field
                for (int i = 0; GUARD(20), i < 20; ++i)
                {
                    amt_out[i + 28] = host_issuer[i];
                    amt_out[i + 8] = host_currency[i];
                }

                // finally create the outgoing txn
                uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
                PREPARE_PAYMENT_SIMPLE_TRUSTLINE(txn_out, amt_out, fee, account_field, source_tag, source_tag);

                uint8_t emithash[32];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting txn failed"), 1);

                trace(SBUF("emit hash: "), SBUF(emithash), 1);

                if (state_set(0, 0, SBUF(STP_REDEEM_OP)) < 0)
                    rollback(SBUF("Evernode: Could not delete state for redeem_op."), 1);

                accept(SBUF("Redeem response successful."), 0);
            }

            // refund
            accept(SBUF("Evernode: XRP transaction."), 1);
        }
        else
        {
            // IOU payment.
            int64_t amt = slot_float(amt_slot);
            if (amt < 0)
                rollback(SBUF("Evernode: Could not parse amount."), 1);

            uint8_t amount_buffer[48];
            if (slot(SBUF(amount_buffer), amt_slot) != 48)
                rollback(SBUF("Evernode: Could not dump sfAmount"), 1);

            // Get amount received in drops
            int64_t amount_val_drops = float_int(amt, 6, 0);
            TRACEVAR(amount_val_drops);

            int is_evr;
            IS_EVR(is_evr, amount_buffer, currency, hook_accid);

            // Start filtering from memos type.
            int is_host_reg_req = 0;
            BUFFER_EQUAL_STR_GUARD(is_host_reg_req, type_ptr, type_len, "evnHostReg", 1);
            if (is_host_reg_req)
            {
                // Currency should be EVR.
                if (!is_evr)
                    rollback(SBUF("Evernode: Currency should be EVR for host registration."), 1);

                if (amount_val_drops < (conf_host_reg_fee * 1000000))
                    rollback(SBUF("Evernode: Amount sent is less than the minimum fee for host registration."), 1);

                // Checking whether this host is already registered.
                HOST_ADDR_KEY(account_field);
                uint8_t host_addr[7]; // <host_id(4)><hosting_token(3)>

                if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) != DOESNT_EXIST)
                    rollback(SBUF("Evernode: Host already registered."), 1);

                int is_format_match = 0;
                BUFFER_EQUAL_STR_GUARD(is_format_match, format_ptr, format_len, "text/plain", 1);
                if (!is_format_match)
                    rollback(SBUF("Evernode: Memo format should be text/plain."), 50);

                int is_dest_hook = 0;
                BUFFER_EQUAL(is_dest_hook, hook_accid, dest_field, 20);
                if (!is_dest_hook)
                    rollback(SBUF("Evernode: Destination should be hook account for host registration."), 2);

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

                uint8_t hosting_token[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, data_ptr[0], data_ptr[1], data_ptr[2], 0, 0, 0, 0, 0};

                uint8_t amt_out[48];
                int64_t token_limit = float_sum(float_set(9, 1), float_negate(float_one())); // 999999999
                // we need to dump the iou amount into a buffer
                // by supplying -1 as the fieldcode we tell float_sto not to prefix an actual STO header on the field
                if (float_sto(SBUF(amt_out), SBUF(hosting_token), SBUF(account_field), token_limit, -1) < 0)
                    rollback(SBUF("Evernode: Could not dump hosting token amount into sto"), 1);

                // set the currency code and issuer in the amount field
                for (int i = 0; GUARD(20), i < 20; ++i)
                {
                    amt_out[i + 28] = account_field[i];
                    amt_out[i + 8] = hosting_token[i];
                }

                // Preparing trustline transaction.
                uint8_t txn_out[PREPARE_SIMPLE_TRUSTLINE_SIZE];
                PREPARE_SIMPLE_TRUSTLINE(txn_out, amt_out, fee, account_field);

                uint8_t emithash[32];
                if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
                    rollback(SBUF("Evernode: Emitting txn failed"), 1);

                trace(SBUF("emit hash: "), SBUF(emithash), 1);

                uint32_t host_id = host_count + 1;
                uint8_t host_id_arr[4];
                UINT32_TO_BUF(host_id_arr, host_id);
                HOST_ID_KEY(host_id_arr);
                if (state_set(SBUF(account_field), SBUF(STP_HOST_ID)) < 0)
                    rollback(SBUF("Evernode: Could not set state for host_id."), 1);

                host_addr[0] = host_id_arr[0];
                host_addr[1] = host_id_arr[1];
                host_addr[2] = host_id_arr[2];
                host_addr[3] = host_id_arr[3];
                host_addr[4] = data_ptr[0];
                host_addr[5] = data_ptr[1];
                host_addr[6] = data_ptr[2];
                if (state_set(SBUF(host_addr), SBUF(STP_HOST_ADDR)) < 0)
                    rollback(SBUF("Evernode: Could not set state for host_addr."), 1);

                host_count += 1;
                UINT32_TO_BUF(host_count_buf, host_count);
                if (state_set(SBUF(host_count_buf), SBUF(STK_HOST_COUNT)) < 0)
                    rollback(SBUF("Evernode: Could not set default state for host count."), 1);

                accept(SBUF("Host registration successful."), 0);
            }

            int is_redeem_req = 0;
            BUFFER_EQUAL_STR_GUARD(is_redeem_req, type_ptr, type_len, "evnRedeem", 1);
            if (is_redeem_req)
            {
                if (is_evr)
                    rollback(SBUF("Evernode: Currency cannot be EVR for redeem request."), 1);

                int is_format_match = 0;
                BUFFER_EQUAL_STR_GUARD(is_format_match, format_ptr, format_len, "binary", 1);
                if (!is_format_match)
                    rollback(SBUF("Evernode: Memo format should be binary."), 50);

                if (amount_val_drops < (conf_min_redeem * 1000000))
                    rollback(SBUF("Evernode: Amount sent is less than the minimum fee."), 1);

                uint8_t issuer[20];
                for (int i = 0; GUARD(20), i < 20; ++i)
                    issuer[i] = amount_buffer[i + 28];

                // Checking whether this host is registered.
                HOST_ADDR_KEY(issuer);
                uint8_t host_addr[7]; // <host_id(4)><hosting_token(3)>

                if (state(SBUF(host_addr), SBUF(STP_HOST_ADDR)) == DOESNT_EXIST)
                    rollback(SBUF("Evernode: Host is not registered."), 1);

                uint8_t host_currency[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, host_addr[4], host_addr[5], host_addr[6], 0, 0, 0, 0, 0};
                int is_htk = 1;
                for (int i = 0; GUARD(20), i < 20; ++i)
                {
                    if (amount_buffer[i + 8] != host_currency[i])
                    {
                        is_evr = 0;
                        break;
                    }
                }

                if (!is_htk)
                    rollback(SBUF("Evernode: Currency should be HTK for host registration."), 1);

                uint8_t txid[32];
                int32_t txid_len = otxn_id(SBUF(txid));
                if (txid_len < 32)
                    rollback(SBUF("Evernode: transaction id missing!!!"), 10);

                uint64_t ledger = ledger_seq();
                uint8_t ledger_buf[8];
                UINT64_TO_BUF(ledger_buf, ledger);

                uint8_t amount_buf[8];
                UINT64_TO_BUF(amount_buf, amt);

                uint8_t redeem_op[39];
                // set the host token
                redeem_op[0] = amount_buffer[20];
                redeem_op[1] = amount_buffer[21];
                redeem_op[2] = amount_buffer[22];
                // set the amount
                for (int i = 0; GUARD(8), i < 8; ++i)
                    redeem_op[i + 3] = amount_buf[i];
                // set the issuer
                for (int i = 0; GUARD(20), i < 20; ++i)
                    redeem_op[i + 11] = issuer[i];
                // set the ledger
                for (int i = 0; GUARD(8), i < 8; ++i)
                    redeem_op[i + 31] = ledger_buf[i];

                REDEEM_OP_KEY(txid);
                if (state_set(SBUF(redeem_op), SBUF(STP_REDEEM_OP)) < 0)
                    rollback(SBUF("Evernode: Could not set state for redeem_op."), 1);

                accept(SBUF("Redeem request successful."), 0);
            }
        }
    }
    else if (txn_type == ttTRUST_SET)
    {
        accept(SBUF("De registration successful."), 0);
    }

    accept(SBUF("Evernode: Transaction type not supported."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
