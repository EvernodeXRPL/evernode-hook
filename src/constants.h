#ifndef CONSTANTS_INCLUDED
#define CONSTANTS_INCLUDED 1

#define HOST_REG "evnHostReg"
#define HOST_DE_REG "evnHostDereg"
#define HEARTBEAT "evnHeartbeat"

#define FORMAT_HEX "hex"
#define FORMAT_BASE64 "base64"
#define FORMAT_TEXT "text/plain"
#define FORMAT_JSON "text/json"

#define EVR_TOKEN "EVR"

#define ttCHECK_CASH 17
#define ttTRUST_SET 20
#define tfSetNoRipple 0x00020000 // Disable rippling on this trust line.

#define MAX_MEMO_SIZE 4096 // Maximum tx blob size.

// Default values.
const uint16_t DEF_MOMENT_SIZE = 72;
const uint16_t DEF_MOMENT_BASE_IDX = 0;
const uint16_t DEF_HOST_HEARTBEAT_FREQ = 1;
const uint64_t DEF_MINT_LIMIT = 72253440;
const int64_t DEF_HOST_REG_FEE_M = 5120;
const int32_t DEF_HOST_REG_FEE_E = 0;
const int64_t DEF_FIXED_REG_FEE_M = 5;
const int32_t DEF_FIXED_REG_FEE_E = 0;
const uint16_t DEF_MAX_REG = 0; // No. of theoretical maximum registrants.
const uint8_t DEF_ISSUER_ADDR[35] = "rfp313DhDv5T3iQbwRhZ9WdXJKx9LaVyJ2"; // This is a hard coded value, can be changed later.
const uint8_t DEF_FOUNDATION_ADDR[35] = "rUWDtXPk4gAp8L6dNS51hLArnwFk4bRxky"; // This is a hard coded value, can be changed later.

// Constants
const uint32_t HOST_ADDR_VAL_SIZE = 75;
const uint32_t AMOUNT_BUF_SIZE = 48;
const uint32_t HASH_SIZE = 32;
const uint32_t COUNTRY_CODE_LEN = 2;
const uint32_t DESCRIPTION_LEN = 26;

// State value offsets
// HOST_ADDR
const uint32_t HOST_TOKEN_OFFSET = 0;
const uint32_t HOST_COUNTRY_CODE_OFFSET = 3;
const uint32_t HOST_CPU_MICROSEC_OFFSET = 5;
const uint32_t HOST_RAM_MB_OFFSET = 9;
const uint32_t HOST_DISK_MB_OFFSET = 13;
const uint32_t HOST_RESERVED_OFFSET = 17;
const uint32_t HOST_DESCRIPTION_OFFSET = 25;
const uint32_t HOST_REG_LEDGER_OFFSET = 51;
const uint32_t HOST_TOT_INS_COUNT_OFFSET = 59;
const uint32_t HOST_ACT_INS_COUNT_OFFSET = 63;
const uint32_t HOST_HEARTBEAT_LEDGER_IDX_OFFSET = 67;

const uint64_t MIN_DROPS = 1;
const char *empty_ptr = 0;

#endif
