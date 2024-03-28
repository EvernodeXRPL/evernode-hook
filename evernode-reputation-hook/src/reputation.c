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
 *  Use sfMessageKey to delegate "reputation scoring authority" from the host account to the reputation account.
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

#define DONE(x)\
    return accept(SBUF(x), __LINE__)

#define NOPE(x)\
    return rollback(SBUF(x), __LINE__)

#define SVAR(x)\
    &(x), sizeof(x)

uint8_t registry_namespace[32] =
{
    0x01U,0xEAU,0xF0U,0x93U,0x26U,0xB4U,0x91U,0x15U,0x54U,0x38U,
    0x41U,0x21U,0xFFU,0x56U,0xFAU,0x8FU,0xECU,0xC2U,0x15U,0xFDU,
    0xDEU,0x2EU,0xC3U,0x5DU,0x9EU,0x59U,0xF2U,0xC5U,0x3EU,0xC6U,
    0x65U,0xA0U
};

uint8_t registry_accid[20] =
{
    0x77U,0xC6U,0xE7U,0x07U,0x43U,0x0AU,0xC2U,0x67U,0x16U,0x12U,
    0x1FU,0x50U,0x53U,0x96U,0x34U,0xE4U,0x02U,0x4FU,0xB4U,0x57U
};

#define MOMENT_SECONDS 3600

int64_t hook(uint32_t r)
{
    _g(1,1);

    if (otxn_type() != ttINVOKE)
        DONE("Everrep: passing non-invoke");

    uint8_t hookacc[20];
    hook_account(hookacc, 20);

    uint8_t accid[28];
    otxn_field(accid + 8, 20, sfAccount);

    if (BUFFER_EQUAL_20(hookacc, accid + 8))
        DONE("Everrep: passing outgoing txn");

    uint8_t blob[64];

    int64_t result = otxn_field(SBUF(blob), sfBlob);

    int64_t no_scores_submitted = (result == DOESNT_EXIST);

    if (!no_scores_submitted && result != 64)
        NOPE("Everrep: sfBlob must be 64 bytes.");

    uint64_t current_moment = ledger_last_time() / MOMENT_SECONDS;
    uint64_t previous_moment = current_moment - 1;
    uint64_t before_previous_moment = current_moment - 2;
    uint64_t next_moment = current_moment + 1;

    uint64_t cleanup_moment[2];
    uint64_t special = 0xFFFFFFFFFFFFFFFFULL;
    state(SVAR(cleanup_moment[0]), SVAR(special));

    // gc

    if (cleanup_moment[0] > 0 && cleanup_moment[0] < before_previous_moment)
    {
        // store the host count in cleanup_moment[1], so we can use the whole array as a key in a minute
        if (state(SVAR(cleanup_moment[1]), SVAR(cleanup_moment[0]) == sizeof(cleanup_moment[1])))
        {

            uint8_t accid[28];
            // we will cleanup the final two entries in an amortised fashion
            if (cleanup_moment[1] > 0)
            {
                *((uint64_t*)accid) = --cleanup_moment[1];
                // fetch the account id for the reverse direction
                state(accid + 8, 20, cleanup_moment, sizeof(cleanup_moment));
                
                state_set(0,0, SBUF(accid));
                state_set(0,0, cleanup_moment, sizeof(cleanup_moment));
            }
            
            if (cleanup_moment[1] > 0)
            {
                *((uint64_t*)accid) = --cleanup_moment[1];
                // fetch the account id for the reverse direction
                state(accid + 8, 20, cleanup_moment, sizeof(cleanup_moment));
                
                state_set(0,0, SBUF(accid));
                state_set(0,0, cleanup_moment, sizeof(cleanup_moment));
            }

            // update or remove moment counters
            if (cleanup_moment[1] > 0)
                state_set(SVAR(cleanup_moment[1]), SVAR(cleanup_moment[0]));
            else
            {
                state_set(0,0, SVAR(cleanup_moment[0]));
                cleanup_moment[0]++;

            }
        }
    }
    state_set(SVAR(cleanup_moment[0]), SVAR(special));


    *((uint64_t*)accid) = previous_moment;
    uint64_t previous_hostid;

    int64_t in_previous_round = (state(SVAR(previous_hostid), SBUF(accid)) == sizeof(previous_hostid));

    if (in_previous_round)
    {    
        if (no_scores_submitted)
            NOPE("Everrep: Submit your scores!");

        // find out which universe you were in
        uint64_t hostid = previous_hostid;

        uint64_t universe = hostid >> 6;

        uint64_t host_count;
        state(SVAR(host_count), accid, 8);

        uint64_t first_hostid = universe << 6;
        
        uint64_t last_hostid = ((universe + 1 ) << 6) - 1;

        uint64_t last_universe = host_count >> 6;

        uint64_t last_universe_hostid = (last_universe << 6) - 1;

        if (hostid <= last_universe_hostid)
        {
            // accumulate the scores
            uint64_t id[2];
            id[0] = previous_moment;
            int n = 0;
            for (id[1] = first_hostid; GUARD(64), id[1] <= last_hostid; ++id[1], ++n)
            {
                uint64_t accid[20];
                if (state(SBUF(accid), id, 16) != 20)
                    continue;
                uint64_t data[3];
                if (state(data, 24, SBUF(accid)) != 24)
                    continue;

                // sanity check: either they are still most recently registered for next moment or last
                if (data[0] > next_moment || data[0] < previous_moment)
                    continue;

                data[1] += blob[n];
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
    state(SBUF(acc_data), accid+8, 20);
    if (acc_data[0] == next_moment)
        NOPE("Everrep: Already registered for this round.");

    acc_data[0] = next_moment;
    if (state_set(acc_data, 24, accid + 8, 20) != 24)
        NOPE("Everrep: Failed to set acc_data. Check hook reserves.");
    
    // first check if they're still in the registry
    uint8_t host_data[256];
    result = state_foreign(SBUF(host_data), accid + 8, 20, SBUF(registry_namespace), SBUF(registry_accid));

    if (result == DOESNT_EXIST)
        DONE("Everrep: Scores submitted but host no longer registered and therefore doesn't qualify for next round.");

    // execution to here means we will register for next round

    uint64_t host_count;
    state(SVAR(host_count), SVAR(next_moment));

    *((uint64_t*)accid) = next_moment;
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
        *((uint64_t*)reverse) = next_moment;
        state(reverse + 8, 20,  forward, 16);

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

    if (no_scores_submitted)
        DONE("Everrep: Registered for next round.");

    DONE("Everrep: Voted and registered for next round.");
    return 0;
}