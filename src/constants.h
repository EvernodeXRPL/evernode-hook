#ifndef CONSTANTS_INCLUDED
#define CONSTANTS_INCLUDED 1

#define HOST_REG "evnHostReg"
#define HOST_DE_REG "evnHostDereg"
#define REDEEM "evnRedeem"
#define REDEEM_REF "evnRedeemRef"
#define REDEEM_RESP "evnRedeemResp"
#define REFUND "evnRefund"
#define AUDIT_REQ "evnAuditRequest"
#define AUDIT_SUCCESS "evnAuditSuccess"

#define FORMAT_BINARY "binary"
#define FORMAT_TEXT "text/plain"
#define FORMAT_JSON "text/json"

#define EVR_TOKEN "EVR"

#define ttCHECK_CREATE 16
#define ttCHECK_CASH 17
#define ttTRUST_SET 20
#define tfSetNoRipple 0x00020000 // Disable rippling on this trust line.

#define MAX_MEMO_SIZE 4096 // Maximum tx blob size.

// Default values.
const uint16_t DEF_MOMENT_SIZE = 72;
const uint64_t DEF_MINT_LIMIT = 25804800;
const uint16_t DEF_HOST_REG_FEE = 5;
const uint16_t DEF_MIN_REDEEM = 12;
const uint16_t DEF_REDEEM_WINDOW = 24;
const uint16_t DEF_REWARD = 64;
const uint16_t DEF_MAX_REWARD = 20;
const uint8_t DEF_AUDITOR_ADDR[35] = "rUWDtXPk4gAp8L6dNS51hLArnwFk4bRxky"; // This is a hard coded value, can be changed later.

// Constants
const int32_t HOST_ADDR_VAL_SIZE = 125;
const int32_t AUDITOR_ADDR_VAL_SIZE = 32;
const int32_t REDEEM_STATE_VAL_SIZE = 39;
const int32_t MOMENT_SEED_VAL_SIZE = 40;
const int32_t AMOUNT_BUF_SIZE = 48;
const int32_t HASH_SIZE = 32;

#endif
