/**
 * Everrep - Reputation accumulator and universe shuffler hook for Evernode.
 *
 * State: (8l = 8 byte uint64 little endian)
 *   [reputation account id : 20b]          => [last registered moment: 8l, score numerator: 8l, score denominator 8l]
 *   [moment : 8l, repaccid : 20b]          => [ordered hostid : 8l]
 *   [moment : 8l, ordered hostid : 8l ]    => [repaccid : 20b]
 *   [moment : 8l]                          => [host count in that moment : 8l]
 *
 * Todo:
 *  Use sfWalletLocator to delegate "reputation scoring authority" from the host account to the reputation account.
 *
 * Behaviour:
 *  There is a reputation round each moment (hour).
 *  To register for a reputation round your reputation account invokes this hook.
 *  If you were in the previous round then you submit your scores as a blob when you invoke, otherwise submit no blob.
 *  You are placed into a shuffled deck. Your Ordered Host ID is your place in the deck. The first place is 0.
 *  Your Ordered host ID is only final when the moment begins.
 *  The deck is divided into subdecks of 64 hosts. These are called Universes.
 *  The first Universe is Ordered Host ID's 0 through 63 inclusive.
 *  Outside of this Hook, the reputation software should ascertain which Universe you are in.
 *  It should compile the reputation contact with a UNL that matches the nodes in that universe.
 *  It should run that contract for around 90% of the moment. Its participation is noticed by the other peers.
 *  Each peer in the universe then compiles the ordered scores into a blob of 64 bytes and submits them back to this
 *  hook to both vote and register for thew next round at the same time.
 */

#include "reputation.h"

#define DONE(x) \
    return accept(SBUF(x), __LINE__)

#define NOPE(x) \
    return rollback(SBUF(x), __LINE__)

#define SVAR(x) \
    &(x), sizeof(x)

int64_t hook(uint32_t reserved)
{
    _g(1, 1);

    if (!ACTIVE(ACTIVATION_UNIXTIME))
        rollback(SBUF("Not active yet."), __LINE__);

    CHECK_PARTIAL_PAYMENT();

    // Getting the hook account id.
    unsigned char hook_accid[20];
    hook_account((uint32_t)hook_accid, 20);

    int64_t cur_ledger_seq = ledger_seq();

    // Fetch the sfAccount field from the originating transaction
    uint8_t account_field[ACCOUNT_ID_SIZE];
    int32_t account_field_len = otxn_field(SBUF(account_field), sfAccount);

    // ASSERT_FAILURE_MSG >> sfAccount field is missing.
    ASSERT(account_field_len == 20);

    enum OPERATION op_type = OP_NONE;
    int64_t txn_type = otxn_type();

    if ((!(txn_type == ttPAYMENT || txn_type == ttINVOKE)) || BUFFER_EQUAL_20(hook_accid, account_field))
    {
        // PERMIT_MSG >> Transaction is not handled.
        PERMIT();
    }

    uint8_t event_type[MAX_EVENT_TYPE_SIZE];
    const int64_t event_type_len = otxn_param(SBUF(event_type), SBUF(PARAM_EVENT_TYPE_KEY));

    // Hook param analysis
    uint8_t event_data[MAX_EVENT_DATA_SIZE];
    const int64_t event_data_len = otxn_param(SBUF(event_data), SBUF(PARAM_EVENT_DATA_KEY));

    int64_t cur_slot = 0;
    int64_t sub_field_slot = 0;

    if (txn_type != ttINVOKE && event_type_len == DOESNT_EXIST)
    {
        // PERMIT_MSG >> Transaction is not handled.
        PERMIT();
    }

    // ASSERT_FAILURE_MSG >> Error getting the event type param.
    ASSERT(txn_type == ttINVOKE || !(event_type_len < 0));

    if (txn_type == ttPAYMENT)
    {
        if (EQUAL_HOOK_UPDATE(event_type, event_type_len))
            op_type = OP_HOOK_UPDATE;
    }
    else if (txn_type == ttINVOKE)
    {

        if (EQUAL_ORPHAN_REPUTATION_ENTRY_REMOVE(event_type, event_type_len))
            op_type = OP_REMOVE_ORPHAN_REPUTATION_ENTRY;
        else if (EQUAL_HOST_SEND_REPUTATION(event_type, event_type_len))
            op_type = OP_HOST_SEND_REPUTATION;
    }

    if (op_type == OP_NONE)
    {
        // PERMIT_MSG >> Transaction is not handled.
        PERMIT();
    }

    // Reading the hook governance account from hook params
    uint8_t state_hook_accid[ACCOUNT_ID_SIZE] = {0};
    // ASSERT_FAILURE_MSG >> Error getting the state hook address from params.
    ASSERT(hook_param(SBUF(state_hook_accid), SBUF(PARAM_STATE_HOOK_KEY)) == ACCOUNT_ID_SIZE);

    int64_t cur_ledger_timestamp = ledger_last_time() + XRPL_TIMESTAMP_OFFSET;

    // <transition index><transition_moment><index_type>
    uint8_t moment_base_info[MOMENT_BASE_INFO_VAL_SIZE] = {0};

    // ASSERT_FAILURE_MSG >> Could not get moment base info state.
    ASSERT(state_foreign(SBUF(moment_base_info), SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF) >= 0);

    uint64_t moment_base_idx = UINT64_FROM_BUF_LE(&moment_base_info[MOMENT_BASE_POINT_OFFSET]);
    uint32_t prev_transition_moment = UINT32_FROM_BUF_LE(&moment_base_info[MOMENT_AT_TRANSITION_OFFSET]);
    // If state does not exist, take the moment type from default constant.
    uint8_t cur_moment_type = moment_base_info[MOMENT_TYPE_OFFSET];
    uint64_t cur_idx = cur_moment_type == TIMESTAMP_MOMENT_TYPE ? cur_ledger_timestamp : cur_ledger_seq;

    // Take the moment size from config.
    uint16_t moment_size = 0;
    GET_CONF_VALUE(moment_size, CONF_MOMENT_SIZE, "Evernode: Could not get moment size.");

    uint64_t current_moment = GET_MOMENT(cur_idx);
    uint64_t previous_moment = current_moment - 1;
    uint64_t before_previous_moment = current_moment - 2;
    uint64_t next_moment = current_moment + 1;

    // Child hook update trigger.
    if (op_type == OP_HOOK_UPDATE)
    {
        // ASSERT_FAILURE_MSG >> This txn has not been initiated via governor hook account.
        ASSERT(BUFFER_EQUAL_20(state_hook_accid, account_field));

        HANDLE_HOOK_UPDATE(CANDIDATE_REPUTATION_HOOK_HASH_OFFSET);
        // NOTE: Above HANDLE_HOOK_UPDATE will be directed to either accept or rollback. Hence no else if block has been introduced the for OP_HOST_SEND_REPUTATIONS.
    }
    else if (op_type == OP_REMOVE_ORPHAN_REPUTATION_ENTRY)
    {
        uint8_t registry_hook_accid[ACCOUNT_ID_SIZE] = {0};

        // ASSERT_FAILURE_MSG >> Could not get registry account id.
        ASSERT(!(state_foreign(SBUF(registry_hook_accid), SBUF(CONF_REGISTRY_ADDR), FOREIGN_REF) < 0));

        // ASSERT_FAILURE_MSG >> This txn has not been initiated via registry hook account.
        ASSERT(BUFFER_EQUAL_20(registry_hook_accid, account_field));

        // Remove corresponding orphan state entries to remove with host removal.
        uint8_t removing_host_accid[20] = {0};
        COPY_20BYTES(removing_host_accid, event_data);

        uint8_t removing_host_acc_keylet[34] = {0};
        util_keylet(SBUF(removing_host_acc_keylet), KEYLET_ACCOUNT, SBUF(removing_host_accid), 0, 0, 0, 0);

        cur_slot = 0;
        sub_field_slot = 0;
        GET_SLOT_FROM_KEYLET(removing_host_acc_keylet, cur_slot);

        // This wallet locater field is a 32 byte buffer: [<20 bytes accid><12 bytes padding>]
        uint8_t host_reputation_account_id[32] = {0};
        sub_field_slot = cur_slot;
        GET_SUB_FIELDS(sub_field_slot, sfWalletLocator, host_reputation_account_id);

        uint8_t reputation_id_state_key[32] = {0};
        COPY_20BYTES(reputation_id_state_key + 12, host_reputation_account_id);

        // Removing host [REPUTATION_ACC_ID] based state.
        uint8_t reputation_id_state_data[24] = {0};
        state(SBUF(reputation_id_state_data), SBUF(reputation_id_state_key));
        state_set(0, 0, SBUF(reputation_id_state_key));

        uint8_t removing_moment_rep_accid_state_key[32] = {0};
        uint8_t removing_moment_order_id_state_key[32] = {0};
        uint8_t order_id[8] = {0};

        uint64_t removing_moment = UINT64_FROM_BUF_LE(&reputation_id_state_data[0]);

        // Removing host [MOMENT + REPUTATION_ACC_ID] based state of before_previous_moment.
        UINT64_TO_BUF_LE(&removing_moment_rep_accid_state_key[4], removing_moment);
        COPY_20BYTES(removing_moment_rep_accid_state_key + 12, host_reputation_account_id);
        state(SBUF(order_id), SBUF(removing_moment_rep_accid_state_key));
        state_set(0, 0, SBUF(removing_moment_rep_accid_state_key));

        // Removing host [MOMENT + ORDER_ID] based state of before_previous_moment.
        UINT64_TO_BUF_LE(&removing_moment_order_id_state_key[16], removing_moment);
        COPY_8BYTES(removing_moment_order_id_state_key + 24, order_id);
        state_set(0, 0, SBUF(removing_moment_order_id_state_key));

        PERMIT();
    }

    // Here onwards OP_HOST_SEND_REPUTATIONS operation will be taken place.

    uint8_t account_id_state_key[32] = {0};
    uint8_t account_id_state_data[24] = {0};
    COPY_20BYTES(account_id_state_key + 12, account_field);

    uint64_t last_rep_registration_moment = (state(SBUF(account_id_state_data), SBUF(account_id_state_key)) != DOESNT_EXIST) ? UINT64_FROM_BUF_LE(account_id_state_data) : 0;

    uint8_t accid[28];
    COPY_20BYTES((accid + 8), account_field);

    if (BUFFER_EQUAL_20(hook_accid, accid + 8))
        DONE("Everrep: passing outgoing txn");

    uint8_t blob[65];

    int64_t result = otxn_field(SBUF(blob), sfBlob);

    int64_t no_scores_submitted = (result == DOESNT_EXIST);

    if (!no_scores_submitted && result != 65)
        NOPE("Everrep: sfBlob must be 65 bytes.");

    uint64_t cleanup_moment[2];
    uint64_t special = 0xFFFFFFFFFFFFFFFFULL;
    state(SVAR(cleanup_moment[0]), SVAR(special));

    // gc

    if (cleanup_moment[0] > 0 && cleanup_moment[0] < before_previous_moment)
    {
        // store the host count in cleanup_moment[1], so we can use the whole array as a key in a minute
        if (state(SVAR(cleanup_moment[1]), SVAR(cleanup_moment[0])) == sizeof(cleanup_moment[1]))
        {

            uint8_t accid[28];
            // we will cleanup the final two entries in an amortized fashion
            if (cleanup_moment[1] > 0)
            {
                *((uint64_t *)accid) = --cleanup_moment[1];
                // fetch the account id for the reverse direction
                state(accid + 8, 20, cleanup_moment, sizeof(cleanup_moment));

                state_set(0, 0, SBUF(accid));
                state_set(0, 0, cleanup_moment, sizeof(cleanup_moment));
            }

            if (cleanup_moment[1] > 0)
            {
                *((uint64_t *)accid) = --cleanup_moment[1];
                // fetch the account id for the reverse direction
                state(accid + 8, 20, cleanup_moment, sizeof(cleanup_moment));

                state_set(0, 0, SBUF(accid));
                state_set(0, 0, cleanup_moment, sizeof(cleanup_moment));
            }

            // update or remove moment counters
            if (cleanup_moment[1] > 0)
                state_set(SVAR(cleanup_moment[1]), SVAR(cleanup_moment[0]));
            else
            {
                state_set(0, 0, SVAR(cleanup_moment[0]));
                cleanup_moment[0]++;
            }
        }
    }
    state_set(SVAR(cleanup_moment[0]), SVAR(special));

    *((uint64_t *)accid) = previous_moment;
    uint64_t previous_hostid;

    int64_t in_previous_round = (state(SVAR(previous_hostid), SBUF(accid)) == sizeof(previous_hostid));

    if (in_previous_round)
    {
        // TODO: Recheck the logic.
        // if (no_scores_submitted)
        //     NOPE("Everrep: Submit your scores!");

        // find out which universe you were in
        uint64_t hostid = previous_hostid;

        uint64_t universe = hostid >> 6;

        uint64_t host_count;
        state(SVAR(host_count), accid, 8);

        uint64_t first_hostid = universe << 6;

        uint64_t last_hostid = ((universe + 1) << 6) - 1;

        uint64_t last_universe = host_count >> 6;

        uint64_t last_universe_hostid = ((last_universe + 1) << 6) - 1;

        if (hostid <= last_universe_hostid)
        {
            // accumulate the scores
            uint64_t id[2];
            id[0] = current_moment;
            int n = 0;
            uint64_t reg_moment = previous_moment;
            for (id[1] = first_hostid; GUARD(64), id[1] <= last_hostid; ++id[1], ++n)
            {
                uint8_t accid[20];
                if (state(SBUF(accid), id, 16) != 20)
                    continue;
                uint64_t data[3];
                if (state(data, 24, SBUF(accid)) != 24)
                    continue;

                reg_moment = data[0];
                // sanity check: either they are still most recently registered for next moment or last
                if (data[0] > next_moment || data[0] < previous_moment)
                    continue;

                data[1] += blob[n + 1];
                data[2]++;

                // when the denominator gets above a certain size we normalize the fraction by dividing top and bottom
                if (data[2] > 12 * host_count)
                {
                    data[1] >>= 1;
                    data[2] >>= 1;
                }
                state_set(data, 24, SBUF(accid));
            }
        }
    }

    // register for the next moment
    // get host voting data
    uint64_t acc_data[3];
    state(SBUF(acc_data), accid + 8, 20);
    if (acc_data[0] == next_moment)
        NOPE("Everrep: Already registered for this round.");

    if (last_rep_registration_moment > 0)
    {
        uint8_t order_id[8] = {0};
        // Clean up Junk state entires related to host previous round.
        uint8_t moment_rep_acc_id_state_key[32];
        UINT64_TO_BUF_LE(&moment_rep_acc_id_state_key[4], last_rep_registration_moment);
        COPY_20BYTES(moment_rep_acc_id_state_key + 12, account_field);
        state(SBUF(order_id), SBUF(moment_rep_acc_id_state_key));
        state_set(0, 0, SBUF(moment_rep_acc_id_state_key));

        uint8_t moment_host_order_id_state_key[32];
        UINT64_TO_BUF_LE(&moment_host_order_id_state_key[16], last_rep_registration_moment);
        COPY_8BYTES(moment_host_order_id_state_key + 24, order_id);
        state_set(0, 0, SBUF(moment_host_order_id_state_key));

        state_set(0, 0, SVAR(last_rep_registration_moment));
    }

    acc_data[0] = next_moment;
    if (state_set(acc_data, 24, accid + 8, 20) != 24)
        NOPE("Everrep: Failed to set acc_data. Check hook reserves.");

    // BEGIN: Check for registration entry.
    uint8_t host_rep_acc_keylet[34] = {0};
    util_keylet(host_rep_acc_keylet, 34, KEYLET_ACCOUNT, account_field, 20, 0, 0, 0, 0);

    // Host Registration State.
    uint8_t host_addr[HOST_ADDR_VAL_SIZE];

    GET_SLOT_FROM_KEYLET(host_rep_acc_keylet, cur_slot);

    uint8_t wallet_locator[32] = {0};
    sub_field_slot = cur_slot;
    GET_SUB_FIELDS(sub_field_slot, sfWalletLocator, wallet_locator);
    HOST_ADDR_KEY(wallet_locator);

    // ASSERT_FAILURE_MSG >> This host is not registered.
    ASSERT(state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) != DOESNT_EXIST);

    // END: Check for registration entry.

    // execution to here means we will register for next round

    uint64_t host_count;
    state(SVAR(host_count), SVAR(next_moment));

    *((uint64_t *)accid) = next_moment;
    uint64_t forward[2] = {next_moment, 0};

    if (host_count == 0)
    {
        if (state_set(accid + 8, 20, forward, 16) != 20 ||
            state_set(SVAR(host_count), SBUF(accid)) != 8)
            NOPE("Everrep: Failed to set state host_count 1. Check hook reserves.");
    }
    else
    {
        // pick a random other host
        uint64_t rnd[4];
        ledger_nonce(rnd, 32);

        uint64_t other = rnd[0] % host_count;

        // put the other at the end
        forward[1] = other;
        uint8_t reverse[28];
        *((uint64_t *)reverse) = next_moment;
        state(reverse + 8, 20, forward, 16);

        forward[1] = host_count;

        if (state_set(SVAR(host_count), SBUF(reverse)) != 8 ||
            state_set(reverse + 8, 20, forward, 16) != 20)
            NOPE("Everrep: Could not set state (move host.) Check hook reserves.");

        // put us where he was
        forward[1] = other;
        if (state_set(SVAR(other), SBUF(accid)) != 8 ||
            state_set(accid + 8, 20, forward, 16) != 20)
            NOPE("Everrep: Could not set state (new host.) Check hook reserves.");
    }

    host_count += 1;
    if (state_set(SVAR(host_count), SVAR(next_moment)) != 8)
        NOPE("Everrep: Failed to set state host_count 1. Check hook reserves.");

    if (no_scores_submitted)
        DONE("Everrep: Registered for next round.");

    DONE("Everrep: Voted and registered for next round.");
    return 0;
}