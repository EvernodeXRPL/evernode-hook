#ifndef CONSTANTS_INCLUDED
#define CONSTANTS_INCLUDED 1

#define HOST_REG "evnHostReg"
#define HOST_DEREG "evnHostDereg"
#define HOST_DEREG_SELF_RES "evnHostDeregSelfRes"
#define HOST_UPDATE_REG "evnHostUpdateReg"
#define HEARTBEAT "evnHeartbeat"
#define INITIALIZE "evnInitialize"
#define DEAD_HOST_PRUNE "evnDeadHostPrune"
#define DEAD_HOST_PRUNE_RES "evnDeadHostPruneRes"
#define HOST_REWARD "evnHostReward"
#define HOST_TRANSFER "evnTransfer"
#define HOST_REBATE "evnHostRebate"
#define HOOK_UPDATE "evnHookUpdate"
#define CANDIDATE_PROPOSE "evnCandidatePropose"
#define CANDIDATE_PROPOSE_REF "evnCandidateProposeRef"
#define CANDIDATE_VOTE "evnCandidateVote"
#define CANDIDATE_PURGED_RES "evnCandidatePurgedRes"
#define CANDIDATE_ACCEPT_RES "evnCandidateAcceptRes"
#define CANDIDATE_REMOVE_RES "evnCandidateRemoveRes"
#define CANDIDATE_WITHDRAW "evnCandidateWithdraw"
#define CANDIDATE_STATUS_CHANGE "evnCandidateStatusChange"
#define CHANGE_GOVERNANCE_MODE "evnGovernanceModeChange"
#define UPDATE_REWARD_POOL "evnRewardPoolUpdate"
#define PENDING_REWARDS_REQUEST "evnPendingRewardReq"
#define HOOK_UPDATE_RES "evnHookUpdateRes"
#define SET_HOOK "evnSetHook"
#define DUD_HOST_REPORT "evnDudHostReport"
#define DUD_HOST_REMOVE "evnDudHostRemove"
#define DUD_HOST_REMOVE_RES "evnDudHostRemoveRes"
#define LINKED_CANDIDATE_REMOVE "evnRemoveLinkedCandidate"
#define ORPHAN_CANDIDATE_REMOVE "evnRemoveOrphanCandidate"
#define HOST_UPDATE_REPUTATION "evnHostUpdateReputation"
#define FOUNDATION_FUND_REQ "evnFoundationFundReq"
#define HOST_REG_FAIL_REFUND "evnHostRegFailRefund"

#define FORMAT_HEX "hex"
#define FORMAT_BASE64 "base64"
#define FORMAT_TEXT "text/plain"
#define FORMAT_JSON "text/json"

#define EVR_TOKEN "EVR"
#define EVR_HOST "evrhost"
#define LEDGER_MOMENT_TYPE 0
#define TIMESTAMP_MOMENT_TYPE 1
#define PENDING_TRANSFER 1

#define NEW_HOOK_CANDIDATE 1
#define PILOTED_MODE_CANDIDATE 2
#define DUD_HOST_CANDIDATE 3

#define STRONG_HOOK 0
#define AGAIN_HOOK 2

#define ttCHECK_CASH 17
#define ttTRUST_SET 20
#define ttHOOK_SET 22
#define ttNFT_MINT 25
#define ttNFT_BURN 26
#define ttNFT_OFFER 27
#define tfSetNoRipple 0x00020000 // Disable rippling on this trust line.
#define tfTransferable 0x00000008
#define tfBurnable 0x00000001
#define tfSellToken 0x00000001
#define tfBuyToken 0x00000000
#define tfOnlyXRP 0x00000002
#define tfHookOverride 0x00000001
#define tfPartialPayment 0x00020000
#define ltHOOK 'H'

#define ltURI_TOKEN 'U'

enum OPERATION
{
    OP_NONE,
    OP_INITIALIZE,
    OP_PROPOSE,
    OP_GOVERNANCE_MODE_CHANGE,
    OP_STATUS_CHANGE,
    OP_WITHDRAW,
    OP_DUD_HOST_REPORT,
    OP_REMOVE_LINKED_CANDIDATE,
    OP_REMOVE,
    OP_REMOVE_ORPHAN_CANDIDATE,
    OP_HOOK_UPDATE,
    OP_CHANGE_CONFIGURATION,
    OP_HEARTBEAT,
    OP_VOTE,
    OP_REWARD_REQUEST,
    OP_HOST_REG,
    OP_HOST_DEREG,
    OP_HOST_UPDATE_REG,
    OP_DEAD_HOST_PRUNE,
    OP_HOST_REBATE,
    OP_HOST_TRANSFER,
    OP_DUD_HOST_REMOVE,
    OP_HOST_REMOVE,
    OP_HOST_UPDATE_REPUTATION,
    OP_FOUNDATION_FUND_REQ,
    OP_HOST_REG_FAIL_REFUND,
    OP_HOST_SEND_REPUTATIONS
};

#define MAX_MEMO_SIZE 4096      // Maximum tx blob size.
#define MAX_HOOK_PARAM_SIZE 256 // Maximum txn param length.
#define MAX_EVENT_TYPE_SIZE 40  // Maximum string length of the event type.

// Maximum size of the event data.
#define MAX_EVENT_DATA_SIZE \
    MAX_HOOK_PARAM_SIZE * 1

// Transfer process related definitions
const uint8_t TRANSFER_FLAG = PENDING_TRANSFER;

// Host flags
// <reputed_on_heartbeat>
const uint8_t REPUTED_ON_HEARTBEAT = 1;

const int64_t XRPL_TIMESTAMP_OFFSET = 946684800;
const int64_t NOW_IN_EVRS = 0.00000001;
const uint8_t NAMESPACE[32] = {0x01, 0xEA, 0xF0, 0x93, 0x26, 0xB4, 0x91, 0x15, 0x54,
                               0x38, 0x41, 0x21, 0xFF, 0x56, 0xFA, 0x8F, 0xEC,
                               0xC2, 0x15, 0xFD, 0xDE, 0x2E, 0xC3, 0x5D, 0x9E,
                               0x59, 0xF2, 0xC5, 0x3E, 0xC6, 0x65, 0xA0}; // sha256('evernode.org|registry')

// Constants
const uint32_t HOST_ADDR_VAL_SIZE = 143;
const uint32_t TOKEN_ID_VAL_SIZE = 124;
const uint32_t TRANSFEREE_ADDR_VAL_SIZE = 60;
const uint32_t AMOUNT_BUF_SIZE = 48;
const uint32_t HASH_SIZE = 32;
const uint32_t NFT_TOKEN_ID_SIZE = 32;
const uint32_t COUNTRY_CODE_LEN = 2;
const uint32_t DESCRIPTION_LEN = 26;
const uint32_t CPU_MODEl_NAME_LEN = 40;
const uint32_t ACCOUNT_ID_SIZE = 20;
const uint32_t REWARD_INFO_VAL_SIZE = 29;
const uint32_t GOVERNANCE_INFO_VAL_SIZE = 70;
const uint32_t REWARD_CONFIGURATION_VAL_SIZE = 20;
const uint32_t MOMENT_TRANSIT_INFO_VAL_SIZE = 11;
const uint32_t MOMENT_BASE_INFO_VAL_SIZE = 13;
const uint32_t EMAIL_ADDRESS_LEN = 40;
const uint32_t REG_NFT_URI_SIZE = 39;
const uint32_t GOVERNANCE_CONFIGURATION_VAL_SIZE = 14;
const uint32_t CANDIDATE_OWNER_VAL_SIZE = 96;
const uint32_t CANDIDATE_ID_VAL_SIZE = 82;
const uint32_t URI_TOKEN_ID_SIZE = 32;
const uint32_t REG_URI_TOKEN_URI_SIZE = 23;
const uint32_t TRX_FEE_BASE_INFO_VAL_SIZE = 18;
const uint32_t NETWORK_CONFIGURATION_VAL_SIZE = 6;

// State value offsets
// REWARD_INFO
const uint32_t EPOCH_OFFSET = 0;
const uint32_t SAVED_MOMENT_OFFSET = 1;
const uint32_t PREV_MOMENT_ACTIVE_HOST_COUNT_OFFSET = 5;
const uint32_t CUR_MOMENT_ACTIVE_HOST_COUNT_OFFSET = 9;
const uint32_t EPOCH_POOL_OFFSET = 13;
const uint32_t HOST_MAX_LEASE_AMOUNT_OFFSET = 21;

// REWARD_CONFIGURATION
const uint32_t EPOCH_COUNT_OFFSET = 0;
const uint32_t FIRST_EPOCH_REWARD_QUOTA_OFFSET = 1;
const uint32_t EPOCH_REWARD_AMOUNT_OFFSET = 5;
const uint32_t REWARD_START_MOMENT_OFFSET = 9;
const uint32_t ACCUMULATED_REWARD_FREQUENCY_OFFSET = 13;
const uint32_t HOST_REPUTATION_THRESHOLD_OFFSET = 15;
const uint32_t HOST_MIN_INSTANCE_COUNT_OFFSET = 16;

// GOVERNANCE_CONFIGURATION
const uint32_t ELIGIBILITY_PERIOD_OFFSET = 0;
const uint32_t CANDIDATE_LIFE_PERIOD_OFFSET = 4;
const uint32_t CANDIDATE_ELECTION_PERIOD_OFFSET = 8;
const uint32_t CANDIDATE_SUPPORT_AVERAGE_OFFSET = 12;

// GOVERNANCE_INFO
const uint32_t GOVERNANCE_MODE_OFFSET = 0;
const uint32_t LAST_CANDIDATE_IDX_OFFSET = 1;
const uint32_t VOTER_BASE_COUNT_OFFSET = 5;
const uint32_t VOTER_BASE_COUNT_CHANGED_TIMESTAMP_OFFSET = 9;
const uint32_t FOUNDATION_LAST_VOTED_CANDIDATE_IDX = 17;
const uint32_t FOUNDATION_LAST_VOTED_TIMESTAMP_OFFSET = 21;
const uint32_t ELECTED_PROPOSAL_UNIQUE_ID_OFFSET = 29;
const uint32_t PROPOSAL_ELECTED_TIMESTAMP_OFFSET = 61;
const uint32_t UPDATED_HOOK_COUNT_OFFSET = 69;

// TRX_FEE_BASE_INFO
const uint32_t FEE_BASE_AVG_OFFSET = 0;
const uint32_t FEE_BASE_AVG_CHANGED_IDX_OFFSET = 4;
const uint32_t FEE_BASE_AVG_ACCUMULATOR_OFFSET = 12;
const uint32_t FEE_BASE_COUNTER_OFFSET = 16;

// HOST_ADDR
const uint32_t HOST_TOKEN_ID_OFFSET = 0;
const uint32_t HOST_COUNTRY_CODE_OFFSET = 32;
const uint32_t HOST_RESERVED_OFFSET = 34;
const uint32_t HOST_DESCRIPTION_OFFSET = 42;
const uint32_t HOST_REG_LEDGER_OFFSET = 68;
const uint32_t HOST_REG_FEE_OFFSET = 76;
const uint32_t HOST_TOT_INS_COUNT_OFFSET = 84;
const uint32_t HOST_ACT_INS_COUNT_OFFSET = 88;
const uint32_t HOST_HEARTBEAT_TIMESTAMP_OFFSET = 92;
const uint32_t HOST_VERSION_OFFSET = 100;
const uint32_t HOST_REG_TIMESTAMP_OFFSET = 103;
const uint32_t HOST_TRANSFER_FLAG_OFFSET = 111;
const uint32_t HOST_LAST_VOTE_CANDIDATE_IDX_OFFSET = 112;
const uint32_t HOST_LAST_VOTE_TIMESTAMP_OFFSET = 116;
const uint32_t HOST_SUPPORT_VOTE_FLAG_OFFSET = 124;
const uint32_t HOST_REPUTATION_OFFSET = 125;
const uint32_t HOST_FLAGS_OFFSET = 126;
const uint32_t HOST_TRANSFER_TIMESTAMP_OFFSET = 127;
const uint32_t HOST_LEASE_AMOUNT_OFFSET = 135;

// TOKEN_ID
const uint32_t HOST_ADDRESS_OFFSET = 0;
const uint32_t HOST_CPU_MODEL_NAME_OFFSET = 20;
const uint32_t HOST_CPU_COUNT_OFFSET = 60;
const uint32_t HOST_CPU_SPEED_OFFSET = 62;
const uint32_t HOST_CPU_MICROSEC_OFFSET = 64;
const uint32_t HOST_RAM_MB_OFFSET = 68;
const uint32_t HOST_DISK_MB_OFFSET = 72;
const uint32_t HOST_EMAIL_ADDRESS_OFFSET = 76;
const uint32_t HOST_ACCUMULATED_REWARD_OFFSET = 116;

// TRANSFEREE_ADDR
const uint32_t TRANSFER_HOST_ADDRESS_OFFSET = 0;
const uint32_t TRANSFER_HOST_LEDGER_OFFSET = 20;
const uint32_t TRANSFER_HOST_TOKEN_ID_OFFSET = 28;

// CANDIDATE_OWNER
const uint32_t CANDIDATE_GOVERNOR_HOOK_HASH_OFFSET = 0;
const uint32_t CANDIDATE_REGISTRY_HOOK_HASH_OFFSET = 32;
const uint32_t CANDIDATE_HEARTBEAT_HOOK_HASH_OFFSET = 64;
const uint32_t CANDIDATE_REPUTATION_HOOK_HASH_OFFSET = 96;


// CANDIDATE_ID
const uint32_t CANDIDATE_OWNER_ADDRESS_OFFSET = 0;
const uint32_t CANDIDATE_IDX_OFFSET = 20;
const uint32_t CANDIDATE_SHORT_NAME_OFFSET = 24;
const uint32_t CANDIDATE_CREATED_TIMESTAMP_OFFSET = 44;
const uint32_t CANDIDATE_PROPOSAL_FEE_OFFSET = 52;
const uint32_t CANDIDATE_POSITIVE_VOTE_COUNT_OFFSET = 60;
const uint32_t CANDIDATE_LAST_VOTE_TIMESTAMP_OFFSET = 64;
const uint32_t CANDIDATE_STATUS_OFFSET = 72;
const uint32_t CANDIDATE_STATUS_CHANGE_TIMESTAMP_OFFSET = 73;
const uint32_t CANDIDATE_FOUNDATION_VOTE_STATUS_OFFSET = 81;

const uint8_t TOKEN_ID_PREFIX[4] = {0, 8, 0, 0}; // In host NFT only tfTransferable flag is set and transfer fee always will be 0.
const uint64_t MIN_DROPS = 1;
const uint32_t NFT_TAXON_M = 384160001;
const uint32_t NFT_TAXON_C = 2459;
const char *empty_ptr = 0;

// MOMENT_TRANSIT_INFO
const uint32_t TRANSIT_IDX_OFFSET = 0;
const uint32_t TRANSIT_MOMENT_SIZE_OFFSET = 8;
const uint32_t TRANSIT_MOMENT_TYPE_OFFSET = 10;

// MOMENT_BASE_INFO
const uint32_t MOMENT_BASE_POINT_OFFSET = 0;
const uint32_t MOMENT_AT_TRANSITION_OFFSET = 8;
const uint32_t MOMENT_TYPE_OFFSET = 12;

// NETWORK_CONFIGURATION
const uint32_t NETWORK_BUSYNESS_DETECT_PERIOD_OFFSET = 0;
const uint32_t NETWORK_BUSYNESS_DETECT_AVERAGE_OFFSET = 4;

const uint32_t DUD_HOST_CANDID_ADDRESS_OFFSET = 12;

// BEGIN : Governance Game related constants.
// Candidate vote statuses
const uint8_t CANDIDATE_REJECTED = 0;
const uint8_t CANDIDATE_SUPPORTED = 1;
const uint8_t CANDIDATE_ELECTED = 2;
const uint8_t CANDIDATE_PURGED = 3;
const uint8_t CANDIDATE_WITHDRAWN = 4;

#define VOTING_COMPLETED(status) \
    (status > CANDIDATE_SUPPORTED)

// Governance modes
const uint8_t PILOTED = 1;
const uint8_t CO_PILOTED = 2;
const uint8_t AUTO_PILOTED = 3;

// Param Offsets
// <hook_keylets(34*3)><unique_id(32)><short_name(20)>
// PROPOSE
const uint32_t CANDIDATE_PROPOSE_KEYLETS_PARAM_OFFSET = 0;
const uint32_t CANDIDATE_PROPOSE_UNIQUE_ID_PARAM_OFFSET = 102;
const uint32_t CANDIDATE_PROPOSE_SHORT_NAME_PARAM_OFFSET = 134;

// <unique_id(32)><vote(1)>
// VOTE
const uint32_t CANDIDATE_VOTE_UNIQUE_ID_PARAM_OFFSET = 0;
const uint32_t CANDIDATE_VOTE_VALUE_PARAM_OFFSET = 32;

// END : Governance Game related constants.
// HOOK_PARAM_KEYS
const uint8_t PARAM_STATE_HOOK_KEY[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
const uint8_t PARAM_EVENT_TYPE_KEY[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2};
const uint8_t PARAM_EVENT_DATA_KEY[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3};

#endif
