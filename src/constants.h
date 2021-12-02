#ifndef CONSTANTS_INCLUDED
#define CONSTANTS_INCLUDED 1

#define HOST_REG "evnHostReg"
#define HOST_DE_REG "evnHostDereg"
#define REDEEM "evnRedeem"
#define REDEEM_ORIGIN "evnRedeemOrigin"
#define REDEEM_REF "evnRedeemRef"
#define REDEEM_SUCCESS "evnRedeemSuccess"
#define REDEEM_ERROR "evnRedeemError"
#define REFUND "evnRefund"
#define REFUND_SUCCESS "evnRefundSuccess"
#define REFUND_ERROR "evnRefundError"
#define AUDIT "evnAudit"
#define AUDIT_ASSIGNMENT "evnAuditAssignment"
#define AUDIT_SUCCESS "evnAuditSuccess"
#define REWARD "evnReward"

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
const uint64_t DEF_MINT_LIMIT = 25804800;
const int64_t DEF_HOST_REG_FEE_M = 87654321;
const int32_t DEF_HOST_REG_FEE_E = -8;
const uint16_t DEF_MIN_REDEEM = 12;
const uint16_t DEF_REDEEM_WINDOW = 24;
const int64_t DEF_REWARD_M = 64;
const int32_t DEF_REWARD_E = 0;
const uint16_t DEF_MAX_REWARD = 20;
const uint8_t DEF_AUDITOR_ADDR[35] = "rUWDtXPk4gAp8L6dNS51hLArnwFk4bRxky"; // This is a hard coded value, can be changed later.

// Constants
const uint32_t HOST_ADDR_VAL_SIZE = 125;
const uint32_t AUDITOR_ADDR_VAL_SIZE = 32;
const uint32_t REDEEM_STATE_VAL_SIZE = 59;
const uint32_t MOMENT_SEED_VAL_SIZE = 40;
const uint32_t AMOUNT_BUF_SIZE = 48;
const uint32_t HASH_SIZE = 32;
const uint32_t HOST_AUDIT_INFO_OFFSET = 109;
const uint32_t INSTANCE_INFO_OFFSET = 39;
const uint32_t INSTANCE_SIZE_LEN = 60;
const uint32_t LOCATION_LEN = 10;
const uint32_t REDEEM_ORIGIN_DATA_LEN = 63;

const uint64_t MIN_DROPS = 1;
const char *empty_ptr = 0;

#endif
