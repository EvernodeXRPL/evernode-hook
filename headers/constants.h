#ifndef CONSTANTS_INCLUDED
#define CONSTANTS_INCLUDED 1

#define HOST_REG "evnHostReg"
#define HOST_DE_REG "evnHostDereg"
#define HOST_UPDATE_REG "evnHostUpdateReg"
#define HEARTBEAT "evnHeartbeat"
#define INITIALIZE "evnInitialize"
#define HOST_POST_DEREG "evnHostPostDereg"
#define DEAD_HOST_PRUNE "evnDeadHostPrune"
#define DEAD_HOST_PRUNE_REF "evnDeadHostPruneRef"
#define HOST_REWARD "evnHostReward"
#define HOST_TRANSFER "evnTransfer"
#define HOST_REBATE "evnHostRebate"
#define HOOK_UPDATE "evnHookUpdate"
#define HOST_REGISTRY_REF "evnHostRegistryRef"

#define FORMAT_HEX "hex"
#define FORMAT_BASE64 "base64"
#define FORMAT_TEXT "text/plain"
#define FORMAT_JSON "text/json"

#define EVR_TOKEN "EVR"
#define EVR_HOST "evrhost"
#define PRUNE_MESSAGE "PRUNED_INACTIVE_HOST"
#define LEDGER_MOMENT_TYPE 0
#define TIMESTAMP_MOMENT_TYPE 1
#define PENDING_TRANSFER 1

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
#define tfHookOveride 0x00000001

#define MAX_MEMO_SIZE 4096 // Maximum tx blob size.

#define STRONG_HOOK 0
#define AGAIN_HOOK 2

#define OP_NONE 0
#define OP_SET_HOOK 10

const int64_t XRPL_TIMESTAMP_OFFSET = 946684800;
const int64_t NOW_IN_EVRS = 0.00000001;
const uint8_t NAMESPACE[32] = {0x01, 0xEA, 0xF0, 0x93, 0x26, 0xB4, 0x91, 0x15, 0x54,
                               0x38, 0x41, 0x21, 0xFF, 0x56, 0xFA, 0x8F, 0xEC,
                               0xC2, 0x15, 0xFD, 0xDE, 0x2E, 0xC3, 0x5D, 0x9E,
                               0x59, 0xF2, 0xC5, 0x3E, 0xC6, 0x65, 0xA0}; // sha256('evernode.org|registry')
const int64_t DEF_EMIT_FEE_THRESHOLD = 1000;                              // In Drops.

// Transition related definitions. Transition state is added on the init transaction if this has >0 value
const uint16_t NEW_MOMENT_SIZE = 3600;
const uint8_t NEW_MOMENT_TYPE = TIMESTAMP_MOMENT_TYPE;

// Transfer process related definitions
const uint8_t TRANSFER_FLAG = PENDING_TRANSFER;

// Constants
const uint32_t HOST_ADDR_VAL_SIZE = 112;
const uint32_t TOKEN_ID_VAL_SIZE = 116;
const uint32_t TRANSFEREE_ADDR_VAL_SIZE = 60;
const uint32_t AMOUNT_BUF_SIZE = 48;
const uint32_t HASH_SIZE = 32;
const uint32_t NFT_TOKEN_ID_SIZE = 32;
const uint32_t COUNTRY_CODE_LEN = 2;
const uint32_t DESCRIPTION_LEN = 26;
const uint32_t CPU_MODEl_NAME_LEN = 40;
const uint32_t ACCOUNT_ID_SIZE = 20;
const uint32_t REWARD_INFO_VAL_SIZE = 21;
const uint32_t REWARD_CONFIGURATION_VAL_SIZE = 13;
const uint32_t MOMENT_TRANSIT_INFO_VAL_SIZE = 11;
const uint32_t MOMENT_BASE_INFO_VAL_SIZE = 13;
const uint32_t EMAIL_ADDRESS_LEN = 40;
const uint32_t REG_NFT_URI_SIZE = 39;

// State value offsets
// REWARD_INFO
const uint32_t EPOCH_OFFSET = 0;
const uint32_t SAVED_MOMENT_OFFSET = 1;
const uint32_t PREV_MOMENT_ACTIVE_HOST_COUNT_OFFSET = 5;
const uint32_t CUR_MOMENT_ACTIVE_HOST_COUNT_OFFSET = 9;
const uint32_t EPOCH_POOL_OFFSET = 13;

// REWARD_CONFIGURATION
const uint32_t EPOCH_COUNT_OFFSET = 0;
const uint32_t FIRST_EPOCH_REWARD_QUOTA_OFFSET = 1;
const uint32_t EPOCH_REWARD_AMOUNT_OFFSET = 5;
const uint32_t REWARD_START_MOMENT_OFFSET = 9;

// HOST_ADDR
const uint32_t HOST_TOKEN_ID_OFFSET = 0;
const uint32_t HOST_COUNTRY_CODE_OFFSET = 32;
const uint32_t HOST_RESERVED_OFFSET = 34;
const uint32_t HOST_DESCRIPTION_OFFSET = 42;
const uint32_t HOST_REG_LEDGER_OFFSET = 68;
const uint32_t HOST_REG_FEE_OFFSET = 76;
const uint32_t HOST_TOT_INS_COUNT_OFFSET = 84;
const uint32_t HOST_ACT_INS_COUNT_OFFSET = 88;
const uint32_t HOST_HEARTBEAT_LEDGER_IDX_OFFSET = 92;
const uint32_t HOST_VERSION_OFFSET = 100;
const uint32_t HOST_REG_TIMESTAMP_OFFSET = 103;
const uint32_t HOST_TRANSFER_FLAG_OFFSET = 111;

// TOKEN_ID
const uint32_t HOST_ADDRESS_OFFSET = 0;
const uint32_t HOST_CPU_MODEL_NAME_OFFSET = 20;
const uint32_t HOST_CPU_COUNT_OFFSET = 60;
const uint32_t HOST_CPU_SPEED_OFFSET = 62;
const uint32_t HOST_CPU_MICROSEC_OFFSET = 64;
const uint32_t HOST_RAM_MB_OFFSET = 68;
const uint32_t HOST_DISK_MB_OFFSET = 72;
const uint32_t HOST_EMAIL_ADDRESS_OFFSET = 76;

// TRANSFEREE_ADDR
const uint32_t TRANSFER_HOST_ADDRESS_OFFSET = 0;
const uint32_t TRANSFER_HOST_LEDGER_OFFSET = 20;
const uint32_t TRANSFER_HOST_TOKEN_ID_OFFSET = 28;

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

#endif
