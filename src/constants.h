#ifndef CONSTANTS_INCLUDED
#define CONSTANTS_INCLUDED 1

#define HOST_REG "evnHostReg"
#define HOST_DE_REG "evnHostDereg"
#define HEARTBEAT "evnHeartbeat"
#define INITIALIZE "evnInitialize"
#define FOUNDATION_REFUND_50 "evnFoundationRefund"

#define FORMAT_HEX "hex"
#define FORMAT_BASE64 "base64"
#define FORMAT_TEXT "text/plain"
#define FORMAT_JSON "text/json"

#define EVR_TOKEN "EVR"
#define EVR_HOST "evrhost"

#define ttCHECK_CASH 17
#define ttTRUST_SET 20
#define ttNFT_MINT 25
#define ttNFT_OFFER 27
#define tfSetNoRipple 0x00020000 // Disable rippling on this trust line.
#define tfTransferable 0x00000008
#define tfSellToken 0x00000001
#define tfBuyToken 0x00000000

#define MAX_MEMO_SIZE 4096 // Maximum tx blob size.

// Default values.
const uint8_t HOOK_INITIALIZER_ADDR[35] = "rnzsYamjXaxAMg4JKp2VWeSWvvuvBaYAzX";
const uint16_t DEF_MOMENT_SIZE = 72;
const uint64_t DEF_MINT_LIMIT = 72253440;
const uint64_t DEF_HOST_REG_FEE = 5120;
const uint64_t DEF_FIXED_REG_FEE = 5;
const uint64_t DEF_MAX_REG = 14112; // No. of theoretical maximum registrants. (72253440/5120)
const uint16_t DEF_HOST_HEARTBEAT_FREQ = 1;
const int32_t DEF_TARGET_PRICE_M = 1;
const int32_t DEF_TARGET_PRICE_E = -2;
// Constants
const uint32_t HOST_ADDR_VAL_SIZE = 115;
const uint32_t TOKEN_ID_VAL_SIZE = 20;
const uint32_t AMOUNT_BUF_SIZE = 48;
const uint32_t HASH_SIZE = 32;
const uint32_t NFT_TOKEN_ID_SIZE = 32;
const uint32_t COUNTRY_CODE_LEN = 2;
const uint32_t DESCRIPTION_LEN = 26;

// State value offsets
// HOST_ADDR
const uint32_t HOST_TOKEN_ID_OFFSET = 0;
const uint32_t HOST_TOKEN_OFFSET = 32;
const uint32_t HOST_COUNTRY_CODE_OFFSET = 35;
const uint32_t HOST_CPU_MICROSEC_OFFSET = 37;
const uint32_t HOST_RAM_MB_OFFSET = 41;
const uint32_t HOST_DISK_MB_OFFSET = 45;
const uint32_t HOST_RESERVED_OFFSET = 49;
const uint32_t HOST_DESCRIPTION_OFFSET = 57;
const uint32_t HOST_REG_LEDGER_OFFSET = 83;
const uint32_t HOST_REG_FEE_OFFSET = 91;
const uint32_t HOST_TOT_INS_COUNT_OFFSET = 99;
const uint32_t HOST_ACT_INS_COUNT_OFFSET = 103;
const uint32_t HOST_HEARTBEAT_LEDGER_IDX_OFFSET = 107;

const uint8_t TOKEN_ID_PREFIX[4] = {0, 8, 0, 0}; // In host NFT only tfTransferable flag is set and transfer fee always will be 0.
const uint64_t MIN_DROPS = 1;
const uint32_t NFT_TAXON_M = 384160001;
const uint32_t NFT_TAXON_C = 2459;
const char *empty_ptr = 0;

#endif
