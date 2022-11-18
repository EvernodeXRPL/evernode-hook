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
    int64_t cur_ledger_seq = ledger_seq();
    /**
     * Calculate corresponding XRPL timestamp.
     * This calculation is based on the UNIX timestamp & XRPL timestamp relationship
     * https://xrpl-hooks.readme.io/reference/ledger_last_time#behaviour
     */
    int64_t cur_ledger_timestamp = ledger_last_time() + XRPL_TIMESTAMP_OFFSET;

    // <transition index><transition_moment><index_type>
    uint8_t moment_base_info[MOMENT_BASE_INFO_VAL_SIZE] = {0};
    int moment_base_info_state_res = state(moment_base_info, MOMENT_BASE_INFO_VAL_SIZE, SBUF(STK_MOMENT_BASE_INFO));
    if (moment_base_info_state_res < 0 && moment_base_info_state_res != DOESNT_EXIST)
        rollback(SBUF("Evernode: Could not get moment base info state."), 1);
    uint64_t moment_base_idx = UINT64_FROM_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);
    uint32_t prev_transition_moment = UINT32_FROM_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]);
    uint8_t cur_moment_type = moment_base_info[MOMENT_TYPE_OFFSET];
    uint64_t cur_idx = cur_moment_type == TIMESTAMP_MOMENT_TYPE ? cur_ledger_timestamp : cur_ledger_seq;

    ///////////////////////////////////////////////////////////////
    /////// Moment transition related logic is handled here ///////

    // <transition_index(uint64_t)><moment_size(uint16_t)><index_type(uint8_t)>
    uint8_t moment_transition_info[MOMENT_TRANSIT_INFO_VAL_SIZE] = {0};
    int transition_state_res = state(moment_transition_info, MOMENT_TRANSIT_INFO_VAL_SIZE, SBUF(CONF_MOMENT_TRANSIT_INFO));
    if (transition_state_res < 0 && transition_state_res != DOESNT_EXIST)
        rollback(SBUF("Evernode: Error getting moment size transaction info state."), 1);

    if (transition_state_res >= 0)
    {
        // Begin : Moment size transition implementation.
        // If there is a transition, transition_idx specifies a index value to perform that.
        uint64_t transition_idx = UINT64_FROM_BUF(&moment_transition_info[TRANSIT_IDX_OFFSET]);
        TRACEVAR(transition_idx);
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
            if (state_set(moment_size_ptr, 2, SBUF(CONF_MOMENT_SIZE)) < 0)
                rollback(SBUF("Evernode: Could not update the state for moment size."), 1);

            // Update the moment base info.
            UINT64_TO_BUF(&moment_base_info[MOMENT_BASE_POINT_OFFSET], converted_transition_idx);
            UINT32_TO_BUF(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET], transition_moment);
            moment_base_info[MOMENT_TYPE_OFFSET] = moment_transition_info[TRANSIT_MOMENT_TYPE_OFFSET];
            if (state_set(SBUF(moment_base_info), SBUF(STK_MOMENT_BASE_INFO)) < 0)
                rollback(SBUF("Evernode: Could not set state for moment base info."), 1);

            // Assign the transition state values with zeros.
            CLEARBUF(moment_transition_info);
            if (state_set(SBUF(moment_transition_info), SBUF(CONF_MOMENT_TRANSIT_INFO)) < 0)
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
        int is_outgoing = 0;
        BUFFER_EQUAL(is_outgoing, hook_accid, account_field, 20);
        if (is_outgoing)
        {
            hook_skip(SBUF(chain_one_hash), 0);
            hook_skip(SBUF(chain_two_hash), 0);

            accept(SBUF("Evernode: Outgoing transaction. Passing."), 0);
        }

        if (txn_type == ttNFT_ACCEPT_OFFER)
        {
            hook_skip(SBUF(chain_one_hash), 0);
            hook_skip(SBUF(chain_two_hash), 0);

            // Memos
            uint8_t memos[MAX_MEMO_SIZE];
            int64_t memos_len = otxn_field(SBUF(memos), sfMemos);

            if (!memos_len)
                accept(SBUF("Evernode: No memos found, Unhandled transaction."), 1);

            uint8_t *memo_ptr, *type_ptr, *format_ptr, *data_ptr;
            uint32_t memo_len, type_len, format_len, data_len;
            GET_MEMO(0, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len);

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

            uint8_t common_params[COMMON_CHAIN_PARAMS_SIZE];
            common_params[CHAIN_IDX_PARAM_OFFSET] = is_xrp ? 2 : 1;
            INT64_TO_BUF(&common_params[CUR_LEDGER_SEQ_PARAM_OFFSET], cur_ledger_seq);
            INT64_TO_BUF(&common_params[CUR_LEDGER_TIMESTAMP_PARAM_OFFSET], cur_ledger_timestamp);
            COPY_BUF(common_params, HOOK_ACCID_PARAM_OFFSET, hook_accid, 0, ACCOUNT_ID_SIZE);
            COPY_BUF(common_params, ACCOUNT_FIELD_PARAM_OFFSET, account_field, 0, ACCOUNT_ID_SIZE);

            if (is_xrp)
            {
                hook_skip(SBUF(chain_one_hash), 0);

                uint8_t chain_two_params[CHAIN_TWO_PARAMS_SIZE];
                UINT64_TO_BUF(&chain_two_params[MOMENT_BASE_IDX_PARAM_OFFSET], moment_base_idx);
                chain_two_params[CUR_MOMENT_TYPE_PARAM_OFFSET] = cur_moment_type;
                UINT64_TO_BUF(&chain_two_params[CUR_IDX_PARAM_OFFSET], cur_idx);
                COPY_BUF(chain_two_params, MOMENT_TRANSITION_INFO_PARAM_OFFSET, moment_transition_info, 0, MOMENT_TRANSIT_INFO_VAL_SIZE);

                if (hook_param_set(SBUF(common_params), SBUF(COMMON_CHAIN_PARAMS), SBUF(chain_two_hash)) < 0)
                    rollback(SBUF("Evernode: Could not set common chain params for chain two."), 1);
                if (hook_param_set(SBUF(chain_two_params), SBUF(CHAIN_TWO_PARAMS), SBUF(chain_two_hash)) < 0)
                    rollback(SBUF("Evernode: Could not set params for chain two."), 1);

                accept(0, 0, 0);
            }
            else
            {
                // IOU payment.

                hook_skip(SBUF(chain_two_hash), 0);

                uint8_t chain_one_params[CHAIN_ONE_PARAMS_SIZE];
                int64_t result = slot(&chain_one_params[AMOUNT_BUF_PARAM_OFFSET], AMOUNT_BUF_SIZE, amt_slot);
                if (result != AMOUNT_BUF_SIZE)
                    rollback(SBUF("Evernode: Could not dump sfAmount"), 1);
                int64_t float_amt = slot_float(amt_slot);
                if (float_amt < 0)
                    rollback(SBUF("Evernode: Could not parse amount."), 1);
                INT64_TO_BUF(&chain_one_params[FLOAT_AMT_PARAM_OFFSET], float_amt);
                COPY_BUF(chain_one_params, TXID_PARAM_OFFSET, txid, 0, HASH_SIZE);

                if (hook_param_set(SBUF(common_params), SBUF(COMMON_CHAIN_PARAMS), SBUF(chain_one_hash)) < 0)
                    rollback(SBUF("Evernode: Could not set common chain params for chain one."), 1);
                if (hook_param_set(SBUF(chain_one_params), SBUF(CHAIN_ONE_PARAMS), SBUF(chain_one_hash)) < 0)
                    rollback(SBUF("Evernode: Could not set params for chain one."), 1);

                accept(0, 0, 0);
            }
        }
        else if (txn_type == ttPAYMENT)
        {
        }
    }

    hook_skip(SBUF(chain_one_hash), 0);
    hook_skip(SBUF(chain_two_hash), 0);
    accept(SBUF("Evernode: Transaction is not handled."), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
