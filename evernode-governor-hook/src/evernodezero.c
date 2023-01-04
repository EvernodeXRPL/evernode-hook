#include "governor.h"

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

    // <transition index><transition_moment><index_type>
    uint8_t moment_base_info[MOMENT_BASE_INFO_VAL_SIZE] = {0};
    int moment_base_info_state_res = state_foreign(moment_base_info, MOMENT_BASE_INFO_VAL_SIZE, SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF);
    if (moment_base_info_state_res < 0 && moment_base_info_state_res != DOESNT_EXIST)
        rollback(SBUF("Evernode: Could not get moment base info state."), 1);
    uint64_t moment_base_idx = UINT64_FROM_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);
    uint32_t prev_transition_moment = UINT32_FROM_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]);
    // If state does not exist, take the moment type from default constant.
    uint8_t cur_moment_type = (moment_base_info_state_res == DOESNT_EXIST) ? DEF_MOMENT_TYPE : moment_base_info[MOMENT_TYPE_OFFSET];
    uint64_t cur_idx = cur_moment_type == TIMESTAMP_MOMENT_TYPE ? cur_ledger_timestamp : cur_ledger_seq;

    ///////////////////////////////////////////////////////////////
    /////// Moment transition related logic is handled here ///////

    // <transition_index(uint64_t)><moment_size(uint16_t)><index_type(uint8_t)>
    uint8_t moment_transition_info[MOMENT_TRANSIT_INFO_VAL_SIZE] = {0};
    int transition_state_res = state_foreign(moment_transition_info, MOMENT_TRANSIT_INFO_VAL_SIZE, SBUF(CONF_MOMENT_TRANSIT_INFO), FOREIGN_REF);
    if (transition_state_res < 0 && transition_state_res != DOESNT_EXIST)
        rollback(SBUF("Evernode: Error getting moment size transaction info state."), 1);

    uint8_t op_type = OP_NONE;

    if (transition_state_res >= 0)
    {
        // Begin : Moment size transition implementation.
        // If there is a transition, transition_idx specifies a index value to perform that.
        uint64_t transition_idx = UINT64_FROM_BUF(&moment_transition_info[TRANSIT_IDX_OFFSET]);
        if (transition_idx > 0 && cur_idx >= transition_idx)
        {
            uint8_t transit_moment_type = moment_transition_info[TRANSIT_MOMENT_TYPE_OFFSET];

            // Take the transition moment
            uint32_t transition_moment;
            GET_MOMENT(transition_moment, transition_idx);

            // Take the transition index.
            // TODO : This can be removed when the moment transition is stable.
            uint32_t converted_transition_idx;
            if (cur_moment_type == transit_moment_type) // Index type hasn't changed, Use the transition index as it is.
                converted_transition_idx = transition_idx;
            else if (cur_moment_type == TIMESTAMP_MOMENT_TYPE) // If transitioning from timestamp to ledgers, Convert transitioning index to ledgers.
            {
                // Time difference.
                const uint64_t diff = cur_idx - transition_idx;
                converted_transition_idx = cur_ledger_seq - (diff / 3);
            }
            else // If transitioning from ledgers to timestamp, Convert transitioning index to timestamp.
            {
                // Ledger difference.
                const uint64_t diff = cur_idx - transition_idx;
                converted_transition_idx = cur_ledger_timestamp - (diff * 3);
            }

            // Add new moment size to the state.
            const uint8_t *moment_size_ptr = &moment_transition_info[TRANSIT_MOMENT_SIZE_OFFSET];
            if (state_foreign_set(moment_size_ptr, 2, SBUF(CONF_MOMENT_SIZE), FOREIGN_REF) < 0)
                rollback(SBUF("Evernode: Could not update the state for moment size."), 1);

            // Update the moment base info.
            UINT64_TO_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET], converted_transition_idx);
            UINT32_TO_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET], transition_moment);
            moment_base_info[MOMENT_TYPE_OFFSET] = moment_transition_info[TRANSIT_MOMENT_TYPE_OFFSET];
            if (state_foreign_set(SBUF(moment_base_info), SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF) < 0)
                rollback(SBUF("Evernode: Could not set state for moment base info."), 1);

            // Assign the transition state values with zeros.
            CLEAR_MOMENT_TRANSIT_INFO(moment_transition_info);
            if (state_foreign_set(SBUF(moment_transition_info), SBUF(CONF_MOMENT_TRANSIT_INFO), FOREIGN_REF) < 0)
                rollback(SBUF("Evernode: Could not set state for moment transition info."), 1);

            moment_base_idx = UINT64_FROM_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);
            prev_transition_moment = UINT32_FROM_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]);
            cur_moment_type = moment_base_info[MOMENT_TYPE_OFFSET];
            cur_idx = cur_moment_type == TIMESTAMP_MOMENT_TYPE ? cur_ledger_timestamp : cur_ledger_seq;
        }
        // End : Moment size transition implementation.
    }

    ///////////////////////////////////////////////////////////////

    uint8_t chain_one_hash[HASH_SIZE];
    if (hook_hash(SBUF(chain_one_hash), 1) < 0)
        rollback(SBUF("Evernode: Could not get the hash of chain one."), 1);

    uint8_t chain_two_hash[HASH_SIZE];
    if (hook_hash(SBUF(chain_two_hash), 2) < 0)
        rollback(SBUF("Evernode: Could not get the hash of chain two."), 1);

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

                memo_lookup = sto_subarray(memos, memos_len, 1);
                if (memo_lookup > 0)
                {
                    uint8_t *memo_ptr1, *type_ptr1, *format_ptr1, *data_ptr1;
                    uint32_t memo_len1, type_len1, format_len1, data_len1;
                    GET_MEMO(memo_lookup, memos, memos_len, memo_ptr1, memo_len1, type_ptr1, type_len1, format_ptr1, format_len1, data_ptr1, data_len1);

                    if (!EQUAL_HOST_REGISTRY_REF(type_ptr1, type_len1))
                        rollback(SBUF("Evernode: Memo type is invalid."), 1);

                    if (!EQUAL_FORMAT_HEX(format_ptr1, format_len1))
                        rollback(SBUF("Evernode: Memo format should be hex for Hook set."), 1);

                    if (hook_param_set(data_ptr1, memo_len1, SBUF(VERIFY_PARAMS), SBUF(chain_two_hash)) < 0)
                        rollback(SBUF("Evernode: Could not set verify params for chain two."), 1);
                }

                // specifically we're interested in the amount sent
                int64_t oslot = otxn_slot(0);
                if (oslot < 0)
                    rollback(SBUF("Evernode: Could not slot originating txn."), 1);

                int64_t amt_slot = slot_subfield(oslot, sfAmount, 0);

                if (reserved == STRONG_HOOK && txn_type == ttPAYMENT)
                {
                    if (amt_slot < 0)
                        rollback(SBUF("Evernode: Could not slot otxn.sfAmount"), 1);

                    int64_t is_xrp = slot_type(amt_slot, 1);
                    if (is_xrp < 0)
                        rollback(SBUF("Evernode: Could not determine sent amount type"), 1);

                    if (is_xrp)
                    {
                        // Host initialization.
                        if (EQUAL_INITIALIZE(type_ptr, type_len))
                        {
                            if (!EQUAL_FORMAT_HEX(format_ptr, format_len))
                                rollback(SBUF("Evernode: Memo format should be hex for initialize."), 1);
                            if (data_len != (2 * ACCOUNT_ID_SIZE))
                                rollback(SBUF("Evernode: Memo data should contain foundation cold wallet and issuer addresses."), 1);
                            op_type = OP_INITIALIZE;
                        }
                        // Host heartbeat.
                        else if (EQUAL_HEARTBEAT(type_ptr, type_len))
                            op_type = OP_HEARTBEAT;
                        // Set hook with HookHashes
                        else if (EQUAL_HOOK_UPDATE(type_ptr, type_len))
                        {
                            if (!EQUAL_FORMAT_HEX(format_ptr, format_len))
                                rollback(SBUF("Evernode: Memo format should be hex for Hook set."), 1);
                            op_type = OP_SET_HOOK;
                        }
                    }
                }

                if (op_type != OP_NONE)
                {
                    uint8_t issuer_accid[ACCOUNT_ID_SIZE] = {0};
                    uint8_t foundation_accid[ACCOUNT_ID_SIZE] = {0};
                    // States does not exists in initialize transaction.
                    if (op_type != OP_INITIALIZE &&
                        (state_foreign(SBUF(issuer_accid), SBUF(CONF_ISSUER_ADDR), FOREIGN_REF) < 0 || state_foreign(SBUF(foundation_accid), SBUF(CONF_FOUNDATION_ADDR), FOREIGN_REF) < 0))
                        rollback(SBUF("Evernode: Could not get issuer or foundation account id."), 1);

                    uint8_t meta_params[META_PARAMS_SIZE];
                    meta_params[OP_TYPE_PARAM_OFFSET] = op_type;
                    INT64_TO_BUF(&meta_params[CUR_LEDGER_SEQ_PARAM_OFFSET], cur_ledger_seq);
                    INT64_TO_BUF(&meta_params[CUR_LEDGER_TIMESTAMP_PARAM_OFFSET], cur_ledger_timestamp);
                    COPY_20BYTES((meta_params + HOOK_ACCID_PARAM_OFFSET), hook_accid);
                    COPY_20BYTES((meta_params + ACCOUNT_FIELD_PARAM_OFFSET), account_field);
                    COPY_20BYTES((meta_params + ISSUER_PARAM_OFFSET), issuer_accid);
                    COPY_20BYTES((meta_params + FOUNDATION_PARAM_OFFSET), foundation_accid);

                    if (data_len > MEMO_PARAM_SIZE)
                        rollback(SBUF("Evernode: No enough space to populate memo data inside a chain param."), 1);

                    uint8_t memo_len = MIN(data_len, MEMO_PARAM_SIZE);

                    if (op_type == OP_INITIALIZE ||
                        op_type == OP_HEARTBEAT)
                    {
                        hook_skip(SBUF(chain_one_hash), 0);
                        meta_params[CHAIN_IDX_PARAM_OFFSET] = 2;

                        uint8_t chain_two_params[CHAIN_TWO_PARAMS_SIZE];
                        UINT64_TO_BUF(&chain_two_params[MOMENT_BASE_IDX_PARAM_OFFSET], moment_base_idx);
                        chain_two_params[CUR_MOMENT_TYPE_PARAM_OFFSET] = cur_moment_type;
                        UINT64_TO_BUF(&chain_two_params[CUR_IDX_PARAM_OFFSET], cur_idx);
                        COPY_MOMENT_TRANSIT_INFO((chain_two_params + MOMENT_TRANSITION_INFO_PARAM_OFFSET), moment_transition_info);

                        if (hook_param_set(SBUF(meta_params), SBUF(META_PARAMS), SBUF(chain_two_hash)) < 0)
                            rollback(SBUF("Evernode: Could not set meta params for chain two."), 1);
                        if (hook_param_set(data_ptr, memo_len, SBUF(MEMO_PARAMS), SBUF(chain_two_hash)) < 0)
                            rollback(SBUF("Evernode: Could not set memo params for chain two."), 1);
                        if (hook_param_set(SBUF(chain_two_params), SBUF(CHAIN_TWO_PARAMS), SBUF(chain_two_hash)) < 0)
                            rollback(SBUF("Evernode: Could not set params for chain two."), 1);

                        accept(0, 0, 0);
                    }
                    else if (op_type == OP_SET_HOOK)
                    {
                        hook_skip(SBUF(chain_two_hash), 0);
                        meta_params[CHAIN_IDX_PARAM_OFFSET] = 1;

                        // Get transaction hash(id).
                        uint8_t txid[HASH_SIZE];
                        int32_t txid_len = otxn_id(SBUF(txid), 0);
                        if (txid_len < HASH_SIZE)
                            rollback(SBUF("Evernode: transaction id missing."), 1);

                        uint8_t chain_one_params[CHAIN_ONE_PARAMS_SIZE];

                        COPY_32BYTES((chain_one_params + TXID_PARAM_OFFSET), txid);

                        if (hook_param_set(SBUF(meta_params), SBUF(META_PARAMS), SBUF(chain_one_hash)) < 0)
                            rollback(SBUF("Evernode: Could not set meta params for chain one."), 1);
                        if (hook_param_set(data_ptr, memo_len, SBUF(MEMO_PARAMS), SBUF(chain_one_hash)) < 0)
                            rollback(SBUF("Evernode: Could not set memo params for chain one."), 1);
                        if (hook_param_set(SBUF(chain_one_params), SBUF(CHAIN_ONE_PARAMS), SBUF(chain_one_hash)) < 0)
                            rollback(SBUF("Evernode: Could not set params for chain one."), 1);

                        accept(0, 0, 0);
                    }
                }
            }
        }
    }

    hook_skip(SBUF(chain_one_hash), 0);
    hook_skip(SBUF(chain_two_hash), 0);
    accept(SBUF("Evernode: Transaction is not handled."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
