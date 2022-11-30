#ifndef CONSTANTS_INCLUDED
#define CONSTANTS_INCLUDED 1

#define HOST_REG "evnHostReg"
#define HOST_DE_REG "evnHostDereg"
#define HOST_UPDATE_REG "evnHostUpdateReg"
#define HEARTBEAT "evnHeartbeat"
#define INITIALIZE "evnInitialize"
#define FOUNDATION_REFUND_50 "evnFoundationRefund"
#define HOST_POST_DEREG "evnHostPostDereg"
#define DEAD_HOST_PRUNE "evnDeadHostPrune"
#define DEAD_HOST_PRUNE_REF "evnDeadHostPruneRef"
#define HOST_REWARD "evnHostReward"
#define HOST_TRANSFER "evnTransfer"
#define HOST_REBATE "evnHostRebate"
#define NEW_SET_HOOK_HASHES "evnNewSetHookHashes"

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
#define MAX_UINT_STR_LEN 20
#define MAX_MEMO_DATA_LEN 454
#define MAX_VERSION_LEN 32

#define STRONG_HOOK 0
#define WEAK_HOOK 1
#define AGAIN_HOOK 2

#define OP_NONE 0
#define OP_INITIALIZE 1
#define OP_HOST_REG 2
#define OP_HOST_DE_REG 3
#define OP_HOST_POST_DEREG 4
#define OP_HOST_UPDATE_REG 5
#define OP_HEARTBEAT 6
#define OP_DEAD_HOST_PRUNE 7
#define OP_HOST_REBATE 8
#define OP_HOST_TRANSFER 9
#define OP_SET_HOOK 10

// Default values.
const uint8_t HOOK_INITIALIZER_ADDR[35] = "rEeFk3SpyCtt8mvjMgaAsvceHHh4nroezM";
const uint16_t DEF_MOMENT_SIZE = 1190;
const uint16_t DEF_MOMENT_TYPE = LEDGER_MOMENT_TYPE;
const uint64_t DEF_MINT_LIMIT = 72253440;
const uint64_t DEF_HOST_REG_FEE = 5120;
const uint64_t DEF_FIXED_REG_FEE = 5;
const uint64_t DEF_MAX_REG = 14112; // No. of theoretical maximum registrants. (72253440/5120)
const uint16_t DEF_HOST_HEARTBEAT_FREQ = 1;
const int32_t DEF_TARGET_PRICE_M = 2;
const int32_t DEF_TARGET_PRICE_E = 0;
const uint16_t DEF_LEASE_ACQUIRE_WINDOW = 160;   // In seconds
const uint16_t DEF_MAX_TOLERABLE_DOWNTIME = 240; // In moments.
const uint8_t DEF_EPOCH_COUNT = 10;
const uint32_t DEF_FIRST_EPOCH_REWARD_QUOTA = 5120;
const uint32_t DEF_EPOCH_REWARD_AMOUNT = 5160960;
const uint32_t DEF_REWARD_START_MOMENT = 0;
const int64_t XRPL_TIMESTAMP_OFFSET = 946684800;

// Transition related definitions. Transition state is added on the init transaction if this has >0 value
const uint16_t NEW_MOMENT_SIZE = 3600;
const uint8_t NEW_MOMENT_TYPE = TIMESTAMP_MOMENT_TYPE;

// Transfer process related definitions
const uint8_t TRANSFER_FLAG = PENDING_TRANSFER;

// Constants
const uint32_t HOST_ADDR_VAL_SIZE = 112;
const uint32_t TOKEN_ID_VAL_SIZE = 76;
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

#define COMMON_CHAIN_PARAMS "common_params"
const uint32_t COMMON_CHAIN_PARAMS_SIZE = 58;
const uint32_t CHAIN_IDX_PARAM_OFFSET = 0;
const uint32_t OP_TYPE_PARAM_OFFSET = 1;
const uint32_t CUR_LEDGER_SEQ_PARAM_OFFSET = 2;
const uint32_t CUR_LEDGER_TIMESTAMP_PARAM_OFFSET = 10;
const uint32_t HOOK_ACCID_PARAM_OFFSET = 18;
const uint32_t ACCOUNT_FIELD_PARAM_OFFSET = 38;

#define CHAIN_ONE_PARAMS "chain_one_params"
const uint32_t CHAIN_ONE_PARAMS_SIZE = 88;
const uint32_t AMOUNT_BUF_PARAM_OFFSET = 0;
const uint32_t FLOAT_AMT_PARAM_OFFSET = 48;
const uint32_t TXID_PARAM_OFFSET = 56;

#define CHAIN_TWO_PARAMS "chain_two_params"
const uint32_t CHAIN_TWO_PARAMS_SIZE = 28;
const uint32_t MOMENT_BASE_IDX_PARAM_OFFSET = 0;
const uint32_t CUR_MOMENT_TYPE_PARAM_OFFSET = 8;
const uint32_t CUR_IDX_PARAM_OFFSET = 9;
const uint32_t MOMENT_TRANSITION_INFO_PARAM_OFFSET = 17;

#endif
