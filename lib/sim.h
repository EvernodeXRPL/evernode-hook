#ifndef SIM_INCLUDED
#define SIM_INCLUDED 1

#include <stdint.h>

#define ACCOUNT_LEN 20
#define HASH_SIZE_1 32

#define FREE_TRANSACTION_OBJ                               \
    {                                                      \
        for (size_t i = 0; i < txn->memos.memo_count; i++) \
        {                                                  \
            if (txn->memos.list[i].type)                   \
            {                                              \
                free(txn->memos.list[i].type);             \
                txn->memos.list[i].type = NULL;            \
            }                                              \
            if (txn->memos.list[i].format)                 \
            {                                              \
                free(txn->memos.list[i].format);           \
                txn->memos.list[i].format = NULL;          \
            }                                              \
            if (txn->memos.list[i].data)                   \
            {                                              \
                free(txn->memos.list[i].data);             \
                txn->memos.list[i].data = NULL;            \
            }                                              \
        }                                                  \
        free(txn->memos.list);                             \
        free(txn);                                         \
    }

struct IOU
{
    uint8_t issuer[ACCOUNT_LEN];
    uint8_t currency[3];
    int64_t amount;
};

struct XRP
{
    int64_t amount;
};

struct Amount
{
    int is_xrp;
    union
    {
        struct IOU iou;
        struct XRP xrp;
    };
};

struct Memo
{
    uint8_t *type;
    uint8_t *format;
    uint8_t *data;
};
struct Memos
{
    struct Memo *list;
    uint8_t memo_count;
};
struct Transaction
{
    uint8_t account[ACCOUNT_LEN];
    struct Amount amount;
    // int is_xrp;
    // int64_t xrp_amount;
    // struct IOU iou_amount;
    uint8_t destination[ACCOUNT_LEN];
    struct Memos memos;
    uint8_t ledger_hash[HASH_SIZE_1];
    uint64_t ledger_index;
};

extern struct Transaction *txn;
extern uint8_t hook_accid[ACCOUNT_LEN];
int64_t hook(int64_t reserved);
void print_bytes(char *heading, uint8_t *write_ptr, uint32_t write_len);
uint64_t get_mantissa(int64_t float1);
int32_t get_exponent(int64_t float1);
int is_negative(int64_t float1);
// C
// These C functions will read the transaction sent from the JS side which was saved in memory by the caller function.

int32_t _g(uint32_t id, uint32_t maxiter);

int64_t accept(uint32_t read_ptr, uint32_t read_len, int64_t error_code);

int64_t etxn_fee_base(uint32_t tx_byte_count);
int64_t etxn_details(uint32_t write_ptr, uint32_t write_len);
int64_t etxn_reserve(uint32_t count);

int64_t float_compare(int64_t float1, int64_t float2, uint32_t mode);
int64_t float_divide(int64_t float1, int64_t float2);
int64_t float_int(int64_t float1, uint32_t decimal_places, uint32_t abs);
int64_t float_set(int32_t exponent, int64_t mantissa);
int64_t float_sum(int64_t float1, int64_t float2);
int64_t float_sto(uint32_t write_ptr, uint32_t write_len, uint32_t cread_ptr, uint32_t cread_len, uint32_t iread_ptr, uint32_t iread_len, int64_t float1, uint32_t field_code);
int64_t float_multiply(int64_t float1, int64_t float2);
int64_t float_negate(int64_t float1);

int64_t hook_account(uint32_t write_ptr, uint32_t write_len);
// May be will need to hardcode the address. Since this is used to detect incomming or outgoing transactions.

// Not sure.
int64_t ledger_seq(void);
int64_t ledger_last_hash(uint32_t write_ptr, uint32_t write_len);

int64_t otxn_field(uint32_t write_ptr, uint32_t write_len, uint32_t field_id);

int64_t otxn_id(uint32_t write_ptr, uint32_t write_len, uint32_t flags);
int64_t otxn_slot(uint32_t slot);
int64_t otxn_type(void);
// 1. Receive transaction as a string format and load it to a struct.

int64_t util_accid(uint32_t write_ptr, uint32_t write_len, uint32_t read_ptr, uint32_t read_len);

int64_t slot(uint32_t write_ptr, uint32_t write_len, uint32_t slot);
int64_t slot_float(uint32_t slot);
int64_t slot_subfield(uint32_t parent_slot, uint32_t field_id, uint32_t new_slot);
int64_t slot_type(uint32_t slot, uint32_t flags);
int64_t slot_set(uint32_t read_ptr, uint32_t read_len, int32_t slot);
int64_t sto_subarray(uint32_t read_ptr, uint32_t read_len, uint32_t array_id);
int64_t sto_subfield(uint32_t read_ptr, uint32_t read_len, uint32_t field_id);

int64_t trace(uint32_t mread_ptr, uint32_t mread_len, uint32_t dread_ptr, uint32_t dread_len, uint32_t as_hex);
int64_t trace_num(uint32_t read_ptr, uint32_t read_len, int64_t number);
int64_t trace_float(uint32_t mread_ptr, uint32_t mread_len, int64_t float1);

// JS

int64_t emit(uint32_t write_ptr, uint32_t write_len, uint32_t read_ptr, uint32_t read_len);

// Reading trustline data.
int64_t util_keylet(uint32_t write_ptr, uint32_t write_len, uint32_t keylet_type, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f);
// 1. send the request to console.
// 2. listen to stdin for response.

int64_t state_set(uint32_t read_ptr, uint32_t read_len, uint32_t kread_ptr, uint32_t kread_len);
// 1. Send the request to JS.
// 2. Keep data in memory until the C program finishes.
// 3. Commit to sqlite based on the return code of C program.

int64_t state(uint32_t write_ptr, uint32_t write_len, uint32_t kread_ptr, uint32_t kread_len);
// 1. Send the request to JS.
// 2. First look in in memory location.
// 3. If not found, look in sqlite.
// 4. If not found, send NOT_FOUND.

int64_t rollback(uint32_t read_ptr, uint32_t read_len, int64_t error_code);

#define SUCCESS 0                // return codes > 0 are reserved for hook apis to return "success"
#define OUT_OF_BOUNDS -1         // could not read or write to a pointer to provided by hook
#define INTERNAL_ERROR -2        // eg directory is corrupt
#define TOO_BIG -3               // something you tried to store was too big
#define TOO_SMALL -4             // something you tried to store or provide was too small
#define DOESNT_EXIST -5          // something you requested wasn't found
#define NO_FREE_SLOTS -6         // when trying to load an object there is a maximum of 255 slots
#define INVALID_ARGUMENT -7      // self explanatory
#define ALREADY_SET -8           // returned when a one-time parameter was already set by the hook
#define PREREQUISITE_NOT_MET -9  // returned if a required param wasn't set before calling
#define FEE_TOO_LARGE -10        // returned if the attempted operation would result in an absurd fee
#define EMISSION_FAILURE -11     // returned if an emitted tx was not accepted by rippled
#define TOO_MANY_NONCES -12      // a hook has a maximum of 256 nonces
#define TOO_MANY_EMITTED_TXN -13 // a hook has emitted more than its stated number of emitted txn
#define NOT_IMPLEMENTED -14      // an api was called that is reserved for a future version
#define INVALID_ACCOUNT -15      // an api expected an account id but got something else
#define GUARD_VIOLATION -16      // a guarded loop or function iterated over its maximum
#define INVALID_FIELD -17        // the field requested is returning sfInvalid
#define PARSE_ERROR -18          // hook asked hookapi to parse something the contents of which was invalid
#define RC_ROLLBACK -19          // used internally by hook api to indicate a rollback
#define RC_ACCEPT -20            // used internally by hook api to indicate an accept
#define NO_SUCH_KEYLET -21       // the specified keylet or keylet type does not exist or could not be computed
#define CANT_RETURN_NEGATIVE -33 // An API would have returned a negative integer except that negative integers are reserved for error codes (i.e. what you are reading.)

#define INVALID_FLOAT -10024 // if the mantissa or exponent are outside normalized ranges

#define KEYLET_HOOK 1
#define KEYLET_HOOK_STATE 2
#define KEYLET_ACCOUNT 3
#define KEYLET_AMENDMENTS 4
#define KEYLET_CHILD 5
#define KEYLET_SKIP 6
#define KEYLET_FEES 7
#define KEYLET_NEGATIVE_UNL 8
#define KEYLET_LINE 9
#define KEYLET_OFFER 10
#define KEYLET_QUALITY 11
#define KEYLET_EMITTED_DIR 12
#define KEYLET_TICKET 13
#define KEYLET_SIGNERS 14
#define KEYLET_CHECK 15
#define KEYLET_DEPOSIT_PREAUTH 16
#define KEYLET_UNCHECKED 17
#define KEYLET_OWNER_DIR 18
#define KEYLET_PAGE 19
#define KEYLET_ESCROW 20
#define KEYLET_PAYCHAN 21
#define KEYLET_EMITTED 22

#define COMPARE_EQUAL 1U
#define COMPARE_LESS 2U
#define COMPARE_GREATER 4U

#include "sfcodes.h"
#include "hookmacro.h"

#endif
