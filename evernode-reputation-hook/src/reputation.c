/**
 * Everrep - Reputation accumulator and universe shuffler hook for Evernode.
 *
 * State: (8l = 8 byte uint64 little endian)
 *   [host account id : 20b]                => [last registered moment: 8l, score numerator: 8l, score denominator 8l, score: 8l, last reset moment: 8l, last scored moment: 8l, universe size 8l]
 *   [moment : 8l, hostaccid : 20b]         => [ordered hostid : 8l]
 *   [moment : 8l, ordered hostid : 8l ]    => [hostaccid : 20b]
 *   [moment : 8l]                          => [host count in that moment : 8l]
 *
 * Behaviour:
 *  There is a reputation round each moment (hour).
 *  To register for a reputation round your reputation account invokes this hook..
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

#define SVAR(x) \
    &(x), sizeof(x)

int64_t hook(uint32_t reserved)
{
    if (!ACTIVE(ACTIVATION_UNIXTIME))
        rollback(SBUF("Not active yet."), __LINE__);

    CHECK_PARTIAL_PAYMENT();

    // Getting the hook account id.
    unsigned char hook_accid[ACCOUNT_ID_SIZE];
    hook_account((uint32_t)hook_accid, ACCOUNT_ID_SIZE);

    int64_t cur_ledger_seq = ledger_seq();

    // Fetch the sfAccount field from the originating transaction
    uint8_t account_field[ACCOUNT_ID_SIZE];
    int32_t account_field_len = otxn_field(SBUF(account_field), sfAccount);

    // ASSERT_FAILURE_MSG >> sfAccount field is missing.
    ASSERT(account_field_len == ACCOUNT_ID_SIZE);

    enum OPERATION op_type = OP_NONE;
    int64_t txn_type = otxn_type();

    if ((!(txn_type == ttPAYMENT || txn_type == ttINVOKE)) || BUFFER_EQUAL_20(hook_accid, account_field))
    {
        // PERMIT_MSG >> Transaction is not handled.
        PERMIT();
    }

    uint8_t event_type[MAX_EVENT_TYPE_SIZE] = {0};
    const int64_t event_type_len = otxn_param(SBUF(event_type), SBUF(PARAM_EVENT_TYPE_KEY));

    if (event_type_len == DOESNT_EXIST)
    {
        // PERMIT_MSG >> Transaction is not handled.
        PERMIT();
    }

    // ASSERT_FAILURE_MSG >> Error getting the event type param.
    ASSERT(!(event_type_len <= 0));

    // Reading the hook governance account from hook params
    uint8_t state_hook_accid[ACCOUNT_ID_SIZE] = {0};
    // ASSERT_FAILURE_MSG >> Error getting the state hook address from params.
    ASSERT(hook_param(SBUF(state_hook_accid), SBUF(PARAM_STATE_HOOK_KEY)) == ACCOUNT_ID_SIZE);

    if (txn_type == ttPAYMENT)
    {
        if (EQUAL_HOOK_UPDATE(event_type, event_type_len))
            op_type = OP_HOOK_UPDATE;
    }
    else if (txn_type == ttINVOKE)
    {
        if (EQUAL_HOST_SEND_REPUTATION(event_type, event_type_len))
            op_type = OP_HOST_SEND_REPUTATION;
    }

    if (op_type == OP_NONE)
    {
        // PERMIT_MSG >> Transaction is not handled.
        PERMIT();
    }

    uint8_t event_data[MAX_EVENT_DATA_SIZE];
    const int64_t event_data_len = otxn_param(SBUF(event_data), SBUF(PARAM_EVENT_DATA_KEY));

    // ASSERT_FAILURE_MSG >> Error getting the event data param.
    ASSERT(!(event_data_len <= 0));

    int64_t cur_ledger_timestamp = ledger_last_time() + XRPL_TIMESTAMP_OFFSET;

    // <transition index><transition_moment><index_type>
    uint8_t moment_base_info[MOMENT_BASE_INFO_VAL_SIZE] = {0};

    // ASSERT_FAILURE_MSG >> Could not get moment base info state.
    ASSERT(state_foreign(SBUF(moment_base_info), SBUF(STK_MOMENT_BASE_INFO), FOREIGN_REF) > 0);

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

    if (op_type == OP_HOOK_UPDATE)
    {
        // ASSERT_FAILURE_MSG >> This txn has not been initiated via governor hook account.
        ASSERT(BUFFER_EQUAL_20(state_hook_accid, account_field));

        uint8_t governance_info[GOVERNANCE_INFO_VAL_SIZE];

        // ASSERT_FAILURE_MSG >> Could not set state for governance_game info.
        ASSERT(state_foreign(SBUF(governance_info), SBUF(STK_GOVERNANCE_INFO), FOREIGN_REF) >= 0);
        const uint8_t updated_hook_count = governance_info[UPDATED_HOOK_COUNT_OFFSET];

        if (updated_hook_count < 3)
        {
            HANDLE_HOOK_UPDATE(CANDIDATE_REPUTATION_HOOK_HASH_OFFSET);
            PERMIT();
        }

        // <owner_address(20)><candidate_idx(4)><short_name(20)><created_timestamp(8)><proposal_fee(8)><positive_vote_count(4)>
        // <last_vote_timestamp(8)><status(1)><status_change_timestamp(8)><foundation_vote_status(1)>
        uint8_t candidate_id[CANDIDATE_ID_VAL_SIZE];
        // <GOVERNOR_HASH(32)><REGISTRY_HASH(32)><HEARTBEAT_HASH(32)><REPUTATION_HASH(32)>
        uint8_t candidate_owner[CANDIDATE_OWNER_VAL_SIZE];

        CANDIDATE_ID_KEY(event_data);

        // ASSERT_FAILURE_MSG >> Error getting a candidate for the given id.
        ASSERT(state_foreign(SBUF(candidate_id), SBUF(STP_CANDIDATE_ID), FOREIGN_REF) > 0);

        // As first 20 bytes of "candidate_id" represents owner address.
        CANDIDATE_OWNER_KEY(candidate_id);

        // ASSERT_FAILURE_MSG >> Could not get candidate owner state.
        ASSERT(state_foreign(SBUF(candidate_owner), SBUF(STP_CANDIDATE_OWNER), FOREIGN_REF) > 0);

        uint8_t hash_arr[HASH_SIZE * 4];
        COPY_32BYTES(hash_arr, &candidate_owner[CANDIDATE_REPUTATION_HOOK_HASH_OFFSET]);

        uint8_t registry_accid[ACCOUNT_ID_SIZE] = {0};
        uint8_t heartbeat_accid[ACCOUNT_ID_SIZE] = {0};

        // ASSERT_FAILURE_MSG >> Could not get heartbeat or registry or reputation hook account id.
        ASSERT(!(state_foreign(SBUF(heartbeat_accid), SBUF(CONF_HEARTBEAT_ADDR), FOREIGN_REF) < 0 ||
                 state_foreign(SBUF(registry_accid), SBUF(CONF_REGISTRY_ADDR), FOREIGN_REF) < 0));

        int tx_size;
        // ASSERT_FAILURE_MSG >> Could not estimate required txn fee.
        ASSERT(!(etxn_reserve(2) < 0));

        uint8_t emithash[HASH_SIZE];

        PREPARE_SET_HOOK_WITH_GRANT_TRANSACTION_TX(hash_arr, NAMESPACE, event_data,
                                                   state_hook_accid,
                                                   &candidate_owner[CANDIDATE_GOVERNOR_HOOK_HASH_OFFSET],
                                                   registry_accid,
                                                   &candidate_owner[CANDIDATE_REGISTRY_HOOK_HASH_OFFSET],
                                                   heartbeat_accid,
                                                   &candidate_owner[CANDIDATE_HEARTBEAT_HOOK_HASH_OFFSET],
                                                   0, 0, 0, 0, 1, tx_size);

        // ASSERT_FAILURE_MSG >> Emitting reputation hook grant update failed.
        ASSERT(emit(SBUF(emithash), SET_HOOK_TRANSACTION, tx_size) == HASH_SIZE);

        PREPARE_HOOK_UPDATE_RES_PAYMENT_TX(1, state_hook_accid, event_data);

        // ASSERT_FAILURE_MSG >> Emitting reputation hook post update trigger failed.
        ASSERT(emit(SBUF(emithash), SBUF(HOOK_UPDATE_RES_PAYMENT)) == HASH_SIZE);

        PERMIT();
    }
    else if (op_type == OP_HOST_SEND_REPUTATION)
    {
        // BEGIN: Check for registration entry.
        uint8_t host_acc_keylet[34] = {0};

        // ASSERT_FAILURE_MSG >> Could not generate the keylet for KEYLET_ACCOUNT.
        ASSERT(util_keylet(SBUF(host_acc_keylet), KEYLET_ACCOUNT, event_data, ACCOUNT_ID_SIZE, 0, 0, 0, 0) == 34);

        int64_t cur_slot = 0;
        GET_SLOT_FROM_KEYLET(host_acc_keylet, cur_slot);

        uint8_t wallet_locator[32] = {0};
        GET_SUB_FIELDS(cur_slot, sfWalletLocator, wallet_locator);

        // ASSERT_FAILURE_MSG >> This account is not set as reputation account for this host.
        ASSERT(BUFFER_EQUAL_20((wallet_locator + 1), account_field));

        // Host Registration State.
        uint8_t host_addr[HOST_ADDR_VAL_SIZE];
        HOST_ADDR_KEY(event_data);
        // ASSERT_FAILURE_MSG >> Error when getting host address.
        ASSERT(!(state_foreign(SBUF(host_addr), SBUF(STP_HOST_ADDR), FOREIGN_REF) < 0));

        // END: Check for registration entry.

        uint8_t moment_accid_key[32] = {0};
        uint8_t *moment_accid = moment_accid_key + 4;
        COPY_20BYTES((moment_accid + 8), event_data);

        uint8_t host_accid_key[32] = {0};
        COPY_20BYTES((host_accid_key + 12), event_data);

        uint8_t blob[66];

        int64_t result = otxn_field(SBUF(blob), sfBlob);

        // ASSERT_FAILURE_MSG >> sfBlob doesn't exist.
        ASSERT(result > 0);
        // ASSERT_FAILURE_MSG >> sfBlob must be 2 bytes or 65 bytes.
        ASSERT(result == 2 || result == 66);
        // ASSERT_FAILURE_MSG >> sfBlob invalid score version.
        ASSERT(blob[1] == REPUTATION_SCORE_VERSION);

        int64_t no_scores_submitted = (result == 2);

        // // TODO: Clarify uncertainty wether this has any affect.
        // uint64_t cleanup_moment[4] = {0};
        // uint64_t special = 0xFFFFFFFFFFFFFFFFULL;
        // uint64_t cleanup_key[4] = {0, 0, 0, special};

        // // ASSERT_FAILURE_MSG >> Error when getting hook state.
        // ASSERT(state(SVAR(cleanup_moment[2]), cleanup_key, 32) >= 0);

        // uint64_t initial_cleanup_moment = cleanup_moment[3];

        // // gc

        // if (cleanup_moment[2] > 0 && cleanup_moment[2] < before_previous_moment)
        // {
        //     // store the host count in cleanup_moment[3], so we can use the whole array as a key in a minute
        //     cleanup_key[3] = cleanup_moment[2];
        //     if (state(SVAR(cleanup_moment[3]), cleanup_key, 32) == sizeof(cleanup_moment[4]))
        //     {
        //         uint8_t cleanup_accid_key[32] = {0};
        //         uint8_t *cleanup_accid = cleanup_accid_key + 4;
        //         // we will cleanup the final two entries in an amortized fashion
        //         if (cleanup_moment[3] > 0)
        //         {
        //             *((uint64_t *)cleanup_accid) = --cleanup_moment[3];
        //             // fetch the account id for the reverse direction
        //             // ASSERT_FAILURE_MSG >> Error when getting hook state.
        //             ASSERT(state(cleanup_accid + 8, 20, cleanup_moment, sizeof(cleanup_moment)) >= 0);
        //             // ASSERT_FAILURE_MSG >> Error when setting hook state.
        //             ASSERT(state_set(0, 0, SBUF(cleanup_accid_key)) >= 0);
        //             ASSERT(state_set(0, 0, cleanup_moment, sizeof(cleanup_moment)) >= 0);
        //         }

        //         if (cleanup_moment[3] > 0)
        //         {
        //             *((uint64_t *)cleanup_accid) = --cleanup_moment[3];
        //             // fetch the account id for the reverse direction
        //             // ASSERT_FAILURE_MSG >> Error when getting hook state.
        //             ASSERT(state(cleanup_accid + 8, 20, cleanup_moment, sizeof(cleanup_moment)) >= 0);
        //             // ASSERT_FAILURE_MSG >> Error when setting hook state.
        //             ASSERT(state_set(0, 0, SBUF(cleanup_accid_key)) >= 0);
        //             ASSERT(state_set(0, 0, cleanup_moment, sizeof(cleanup_moment)) >= 0);
        //         }

        //         cleanup_key[3] = cleanup_moment[2];
        //         // update or remove moment counters
        //         if (cleanup_moment[3] > 0)
        //         {
        //             // ASSERT_FAILURE_MSG >> Error when setting hook state.
        //             ASSERT(state_set(SVAR(cleanup_moment[3]), cleanup_key, 32) >= 0);
        //         }
        //         else
        //         {
        //             // ASSERT_FAILURE_MSG >> Error when setting hook state.
        //             ASSERT(state_set(0, 0, cleanup_key, 32) >= 0);
        //             cleanup_moment[2]++;
        //         }
        //     }
        // }

        // // Only do state_set if cleanup_moment[0] was updated since reading it initially
        // if (cleanup_moment[2] != initial_cleanup_moment)
        // {
        //     cleanup_key[3] = special;
        //     ASSERT(state_set(SVAR(cleanup_moment[2]), cleanup_key, 32) >= 0);
        // }

        uint64_t moment_key[4] = {0, 0, 0, current_moment};
        *((uint64_t *)moment_accid) = current_moment;

        uint64_t previous_hostid;

        int in_previous_round = (state(SVAR(previous_hostid), SBUF(moment_accid_key)) == sizeof(previous_hostid));

        if (in_previous_round && !no_scores_submitted)
        {
            // find out which universe you were in
            uint64_t hostid = previous_hostid;

            uint64_t universe = hostid >> 6;

            uint64_t host_count;
            // ASSERT_FAILURE_MSG >> Error when getting hook state.
            ASSERT(state(SVAR(host_count), moment_key, 32) >= 0);

            uint64_t first_hostid = universe << 6;

            uint64_t last_hostid = ((universe + 1) << 6) - 1;

            uint64_t last_universe = host_count >> 6;

            uint64_t last_universe_hostid = ((last_universe + 1) << 6) - 1;

            if (hostid <= last_universe_hostid)
            {
                // Get the host count in the universe.
                uint64_t universe_size = REPUTATION_UNIVERSE_SIZE;
                if (universe == last_universe && (host_count % REPUTATION_UNIVERSE_SIZE) != 0)
                    universe_size = host_count % REPUTATION_UNIVERSE_SIZE;

                // accumulate the scores
                uint64_t id[4] = {0, 0, current_moment, 0};
                int n = 0;
                for (id[3] = first_hostid; GUARD(REPUTATION_UNIVERSE_SIZE), id[3] <= last_hostid; ++id[3], ++n)
                {
                    uint8_t accid_key[32] = {0};
                    uint8_t *accid = accid_key + 12;
                    if (state(accid, 20, id, 32) != 20)
                        continue;
                    uint64_t data[7];
                    if (!(state(SBUF(data), SBUF(accid_key)) >= 24))
                        continue;

                    // sanity check: either they are still most recently registered for next moment or last
                    if (data[0] > next_moment || data[0] < previous_moment)
                        continue;

                    // If this is a new moment.
                    if (current_moment > data[4])
                    {
                        // If we receive the minimum number of votes. update the score.
                        if (data[4] != 0 && data[6] != 0 && data[2] >= MIN_DENOM_REQUIREMENT(data[6]))
                        {
                            data[3] = (data[5] == 0) ? data[1] / data[2] : (data[3] + (data[1] / data[2])) / 2;
                            data[5] = current_moment;
                        }
                        data[4] = current_moment;
                        data[1] = 0;
                        data[2] = 0;
                    }

                    data[1] += blob[n + 2];
                    data[2]++;
                    data[6] = universe_size;

                    state_set(SBUF(data), SBUF(accid_key));
                }
            }
        }

        // register for the next moment
        // get host voting data
        uint64_t acc_data[7] = {0};
        int res = state(SBUF(acc_data), SBUF(host_accid_key));
        // ASSERT_FAILURE_MSG >> Error when getting hook state.
        ASSERT(res > 0 || res == DOESNT_EXIST);
        // ASSERT_FAILURE_MSG >> Already registered for this round.
        ASSERT(acc_data[0] != next_moment);

        if (acc_data[0] > 0)
        {
            // Clean up Junk state entires related to host previous round.
            uint64_t cleanup_moment = acc_data[0] == current_moment ? acc_data[0] - 1 : acc_data[0];
            uint64_t order_id[4] = {0, 0, cleanup_moment, 0};
            *((uint64_t *)moment_accid) = cleanup_moment;
            if (state(SVAR(order_id[3]), SBUF(moment_accid_key)) > 0)
            {
                state_set(0, 0, SBUF(moment_accid_key));
                state_set(0, 0, order_id, 32);
            }

            moment_key[3] = cleanup_moment;
            state_set(0, 0, moment_key, 32);

            // TODO: This section is used to cleanup older deprecated states from v0.8.3. Can be removed when all hosts are updated and stabilized.
            uint8_t deprecated_accid_key[32] = {0};
            uint8_t *deprecated_accid = deprecated_accid_key + 4;
            *((uint64_t *)deprecated_accid) = cleanup_moment;
            COPY_20BYTES((deprecated_accid + 8), account_field);
            if (state(SVAR(order_id[3]), SBUF(deprecated_accid_key)) > 0)
            {
                state_set(0, 0, SBUF(deprecated_accid_key));
                state_set(0, 0, order_id, 32);
            }
            ///////////////////////

            // If we have missed a round, There's a gap between moments. So cleanup previous.
            if (cleanup_moment == acc_data[0])
            {
                cleanup_moment--;
                order_id[2] = cleanup_moment;
                *((uint64_t *)moment_accid) = cleanup_moment;
                if (state(SVAR(order_id[3]), SBUF(moment_accid_key)) > 0)
                {
                    state_set(0, 0, SBUF(moment_accid_key));
                    state_set(0, 0, order_id, 32);
                }
                moment_key[3] = cleanup_moment;
                state_set(0, 0, moment_key, 32);

                // TODO: This section is used to cleanup older deprecated states from v0.8.3. Can be removed when all hosts are updated and stabilized.
                *((uint64_t *)deprecated_accid) = cleanup_moment;
                if (state(SVAR(order_id[3]), SBUF(deprecated_accid_key)) > 0)
                {
                    state_set(0, 0, SBUF(deprecated_accid_key));
                    state_set(0, 0, order_id, 32);
                }
            }
            ///////////////////////
        }

        acc_data[0] = next_moment;
        // Initialize reset moment if empty.
        if (acc_data[4] == 0)
            acc_data[4] = current_moment;
        // ASSERT_FAILURE_MSG >> Failed to set acc_data. Check hook reserves.
        ASSERT(state_set(SBUF(acc_data), SBUF(host_accid_key)) == 56);

        // execution to here means we will register for next round

        uint64_t host_count = 0;
        moment_key[3] = next_moment;
        // ASSERT_FAILURE_MSG >> Error when getting hook state.
        res = state(SVAR(host_count), moment_key, 32);
        ASSERT(res > 0 || res == DOESNT_EXIST);

        *((uint64_t *)moment_accid) = next_moment;
        uint64_t forward[4] = {0, 0, next_moment, 0};

        if (host_count == 0)
        {
            // ASSERT_FAILURE_MSG >> Failed to set state host_count 1. Check hook reserves.
            ASSERT(state_set(moment_accid + 8, 20, forward, 32) == 20 &&
                   state_set(SVAR(host_count), SBUF(moment_accid_key)) == 8);
        }
        else
        {
            // pick a random other host
            uint64_t rnd[4];
            // ASSERT_FAILURE_MSG >> Error when getting nonce.
            ASSERT(ledger_nonce(rnd, 32) >= 0);

            uint64_t other = rnd[0] % host_count;

            // put the other at the end
            forward[3] = other;
            uint8_t reverse_key[32] = {0};
            uint8_t *reverse = reverse_key + 4;
            *((uint64_t *)reverse) = next_moment;
            // ASSERT_FAILURE_MSG >> Could not set state.
            ASSERT(state(reverse + 8, 20, forward, 32) >= 0);

            forward[3] = host_count;

            // ASSERT_FAILURE_MSG >> Could not set state (move host.) Check hook reserves.
            ASSERT(state_set(SVAR(host_count), SBUF(reverse_key)) == 8 &&
                   state_set(reverse + 8, 20, forward, 32) == 20);

            // put us where he was
            forward[3] = other;
            // ASSERT_FAILURE_MSG >> Could not set state (new host.) Check hook reserves.
            ASSERT(state_set(SVAR(other), SBUF(moment_accid_key)) == 8 &&
                   state_set(moment_accid + 8, 20, forward, 32) == 20);
        }

        host_count += 1;
        // ASSERT_FAILURE_MSG >> Failed to set state host_count 1. Check hook reserves.
        ASSERT(state_set(SVAR(host_count), SVAR(next_moment)) == 8);

        // PERMIT_MSG >> Registered for next round.
        PERMIT()
    }

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}