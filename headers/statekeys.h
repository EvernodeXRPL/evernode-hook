#ifndef STATEKEYS_INCLUDED
#define STATEKEYS_INCLUDED 1

/////////// Singelton keys. ///////////

// Host count (Maintains total no. of registered hosts)
// value 50 is in decimal. Its converted to 32 in hex.
const uint8_t STK_HOST_COUNT[32] = {'E', 'V', 'R', 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Ledger index when the CONF_MOMENT_SIZE last changed on.
// After the moment transition, this is a composite state which contains <transition index(uint64_t)><transition_moment(uint32_t)><index_type(uint8_t)>
const uint8_t STK_MOMENT_BASE_INFO[32] = {'E', 'V', 'R', 51, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// The host registration fee in Evers.
const uint8_t STK_HOST_REG_FEE[32] = {'E', 'V', 'R', 52, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// No. of theoretical maximum registrants.
const uint8_t STK_MAX_REG[32] = {'E', 'V', 'R', 53, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Epoch reward info <epoch(uint8_t)><saved_moment(uint32_t)><prev_moment_active_host_count(uint32_t)><cur_moment_active_host_count(uint32_t)><epoch_pool(int64_t,xfl)>.
const uint8_t STK_REWARD_INFO[32] = {'E', 'V', 'R', 54, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Governance info <governance_mode(unit8_t)><last_candidate_idx(unit32_t)><voter_base_count(unit32_t)><voter_base_count_changed_timestamp(unit64_t)>
// <foundation_last_voted_candidate_idx(unit32_t)><foundation_last_voted_timestamp(unit64_t)><elected_proposal_unique_id(32)><proposal_elected_timestamp(unit64_t)><updated_hook_count(unit8_t)>
const uint8_t STK_GOVERNANCE_INFO[32] = {'E', 'V', 'R', 55, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Transaction fee base info <fee_base_avg(uint32_t)><avg_changed_idx(uint64_t)><avg_accumulator(uint32_t)><counter(uint16_t)>
const uint8_t STK_TRX_FEE_BASE_INFO[32] = {'E', 'V', 'R', 56, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/////////// Repetitive state keys. ///////////

// Token id keys (Host registration entries for token id-based lookup).
uint8_t STP_TOKEN_ID[32] = {'E', 'V', 'R', 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Host address keys (Host registration entries for xrpl address-based lookup). Last 20 bytes will be replaced by host address in runtime.
uint8_t STP_HOST_ADDR[32] = {'E', 'V', 'R', 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Pending Transfers (Host transfer initiations for transferee address-based lookup)
uint8_t STP_TRANSFEREE_ADDR[32] = {'E', 'V', 'R', 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Candidate owner keys. (Hook candidate proposal info for xrpl address-based lookup). Last 20 bytes will be replaced by owner address in runtime.
uint8_t STP_CANDIDATE_OWNER[32] = {'E', 'V', 'R', 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Hook Candidate Id keys. (Hook candidate proposal entries for candidate id-based lookup).
uint8_t STP_CANDIDATE_ID[32] = {'E', 'V', 'R', 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/////////// Hook Configuration. ///////////
// All configuration keys has the prefix STP_CONF = 1;

// Issuer address.
const uint8_t CONF_ISSUER_ADDR[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
// Foundation address
const uint8_t CONF_FOUNDATION_ADDR[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};
// Seconds per moment.
const uint8_t CONF_MOMENT_SIZE[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3};
// No. of Evers that will be ever issued.
const uint8_t CONF_MINT_LIMIT[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4};
// Portion of registration fee forfeit by the foundation.
const uint8_t CONF_FIXED_REG_FEE[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5};
// Moment frequency which host should keep signaling the registry contract (which used to track host aliveness).
const uint8_t CONF_HOST_HEARTBEAT_FREQ[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6};
// Lease acquire ledger window.
const uint8_t CONF_LEASE_ACQUIRE_WINDOW[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7};
// Reward configuration <epoch_count(uint8_t)><first_epoch_reward_quota(uint32_t)><epoch_reward_amount(uint32_t)><reward_start_moment(uint32_t)><accumulated_reward_frequency(uint16_t)>.
const uint8_t CONF_REWARD_CONFIGURATION[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8};
// The maximum tolerable downtime for a host.
const uint8_t CONF_MAX_TOLERABLE_DOWNTIME[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9};
// Scheduled moment size transition info <transition_index(uint64_t)><moment_size(uint16_t)><index_type(uint_8)>.
const uint8_t CONF_MOMENT_TRANSIT_INFO[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10};
// The maximum fee Hook can bear for a transaction emission.
// To mitigate consuming XRPs unnecessarily due to the execution of Hooks that might be in destination accounts.
const uint8_t CONF_MAX_EMIT_TRX_FEE[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11};
// Heartbeat Hook address.
const uint8_t CONF_HEARTBEAT_ADDR[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12};
// Registry Hook address.
const uint8_t CONF_REGISTRY_ADDR[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13};
// Governance configuration for proposing/voting <eligibility_period(uint32_t)><candidate_life_period(uint32_t)><candidate_election_period(uint32_t)>
// <candidate_support_average(uint16_t)>.
const uint8_t CONF_GOVERNANCE_CONFIGURATION[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14};
// Network configuration <busyness_detect_period(uint32_t)><busyness_detect_average(uint16_t)>.
const uint8_t CONF_NETWORK_CONFIGURATION[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15};

#define HOST_ADDR_KEY(host_addr) \
    COPY_20BYTES((STP_HOST_ADDR + 12), host_addr)

#define TOKEN_ID_KEY(token_id)                         \
    COPY_8BYTES((STP_TOKEN_ID + 4), (token_id + 4));   \
    COPY_8BYTES((STP_TOKEN_ID + 12), (token_id + 12)); \
    COPY_8BYTES((STP_TOKEN_ID + 20), (token_id + 20)); \
    COPY_4BYTES((STP_TOKEN_ID + 28), (token_id + 28));

#define TRANSFEREE_ADDR_KEY(transferee_addr) \
    COPY_20BYTES((STP_TRANSFEREE_ADDR + 12), transferee_addr)

#define CANDIDATE_OWNER_KEY(owner_address) \
    COPY_20BYTES((STP_CANDIDATE_OWNER + 12), owner_address)

#define CANDIDATE_ID_KEY(candidate_id)                         \
    COPY_8BYTES((STP_CANDIDATE_ID + 4), (candidate_id + 4));   \
    COPY_8BYTES((STP_CANDIDATE_ID + 12), (candidate_id + 12)); \
    COPY_8BYTES((STP_CANDIDATE_ID + 20), (candidate_id + 20)); \
    COPY_4BYTES((STP_CANDIDATE_ID + 28), (candidate_id + 28));

#endif