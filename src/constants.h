#ifndef CONSTANTS_INCLUDED
#define CONSTANTS_INCLUDED 1

#define HOST_REG "evnHostReg"
#define HOST_DE_REG "evnHostDereg"
#define INITIALIZE "evnInitialize"

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
const uint8_t ISSUER_ADDR[35] = "r35PTP1rmshmhp7dsMLmJsyftBxXgNBTH6";
const uint8_t FOUNDATION_ADDR[35] = "rnK3nF9TdzNmMJZmACo23Zva5a4f6hFoEe";
const uint16_t DEF_MOMENT_SIZE = 72;
const uint64_t DEF_MINT_LIMIT = 72253440;
const uint64_t DEF_HOST_REG_FEE = 5120;
const uint64_t DEF_FIXED_REG_FEE = 5;
const uint16_t DEF_MAX_REG = 14112; // No. of theoretical maximum registrants. (72253440/5120)
const uint16_t DEF_HOST_HEARTBEAT_FREQ = 1;

// Constants
const uint32_t HOST_ADDR_VAL_SIZE = 115;;
const uint32_t AMOUNT_BUF_SIZE = 48;
const uint32_t HASH_SIZE = 32;
const uint32_t COUNTRY_CODE_LEN = 2;
const uint32_t DESCRIPTION_LEN = 26;

// State value offsets
// HOST_ADDR
const uint32_t HOST_TOKEN_OFFSET = 4;
const uint32_t HOST_COUNTRY_CODE_OFFSET = 7;
const uint32_t HOST_CPU_MICROSEC_OFFSET = 9;
const uint32_t HOST_RAM_MB_OFFSET = 13;
const uint32_t HOST_DISK_MB_OFFSET = 17;
const uint32_t HOST_RESERVED_OFFSET = 21;
const uint32_t HOST_DESCRIPTION_OFFSET = 29;

const uint64_t MIN_DROPS = 1;
const char *empty_ptr = 0;

#endif
