#ifndef SIM_INCLUDED
#define SIM_INCLUDED 1

#include <stdint.h>

#define ACCOUNT_LEN 20
#define HASH_SIZE_1 32

#define FREE_TRANSACTION_OBJ                               \
    if (txn)                                               \
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

#define FREE_SLOTS                            \
    if (sl_count > 0)                         \
    {                                         \
        for (size_t i = 0; i < sl_count; i++) \
            free(slots_arr[i]);               \
    }

#define CLEAN_EMULATOR   \
    FREE_TRANSACTION_OBJ \
    FREE_SLOTS

struct IOU
{
    uint8_t issuer[ACCOUNT_LEN];
    uint8_t currency[20];
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
    uint8_t type_len;
    uint8_t *format;
    uint8_t format_len;
    uint8_t *data;
    uint8_t data_len;
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
    uint8_t destination[ACCOUNT_LEN];
    struct Memos memos;
    uint8_t ledger_hash[HASH_SIZE_1];
    uint64_t ledger_index;
};

struct Trustline
{
    uint8_t high_account[ACCOUNT_LEN];
    uint8_t low_account[ACCOUNT_LEN];
    int64_t limit;
    struct Amount amount;
};

enum MESSAGE_TYPES
{
    TRACE,
    EMIT,
    KEYLET,
    STATE_GET,
    STATE_SET,
    ACCID,
    SEQUENCE
};

enum RETURN_CODES
{
    RES_SUCCESS = 0,
    RES_INTERNAL_ERROR = -1,
    RES_NOT_FOUND = -2,
    RET_OVERFLOW = -3,
    RET_UNDERFLOW = -4
};

struct etxn_info
{
    uint32_t etxn_reserves;
    int etxn_reserved;
};

extern struct Transaction *txn;
extern uint8_t hook_accid[ACCOUNT_LEN];
extern int64_t slots_arr[255];
extern uint8_t sl_count;

int read_stdin(uint8_t *read_buf, const int read_len);

int64_t hook(int64_t reserved);

/**
 * Guard function. Each time a loop appears in your code a call to this must be the first branch instruction after the
 * beginning of the loop.
 * @param id The identifier of the guard (typically the line number).
 * @param maxiter The maximum number of times this loop will iterate across the life of the hook.
 * @return Can be ignored. If the guard is violated the hook will terminate.
 */
int32_t _g(uint32_t id, uint32_t maxiter);

/**
 * Accept the originating transaction and commit all hook state changes and submit all emitted transactions.
 * @param read_ptr An optional string to use as a return comment. May be 0.
 * @param read_len The length of the string. May be 0.
 * @return Will never return, terminates the hook.
 */
int64_t accept(uint32_t read_ptr, uint32_t read_len, int64_t error_code);

/**
 * Rollback the originating transaction, discard all hook state changes and emitted transactions.
 * @param read_ptr An optional string to use as a return comment. May be 0.
 * @param read_len The length of the string. May be 0.
 * @return Will never return, terminates the hook.
 */
int64_t rollback(uint32_t read_ptr, uint32_t read_len, int64_t error_code);

/**
 * Compute the minimum fee required to be paid by a hypothetically emitted transaction based on its size in bytes.
 * @param The size of the emitted transaction in bytes
 * @return The minimum fee in drops this transaction should pay to succeed
 */
int64_t etxn_fee_base(uint32_t tx_byte_count);

/**
 * Write a full emit_details stobject into the buffer specified.
 * @param write_ptr A sufficiently large buffer to write into.
 * @param write_len The length of that buffer.
 * @return The number of bytes written or a negative integer indicating an error.
 */
int64_t etxn_details(uint32_t write_ptr, uint32_t write_len);

/**
 * Inform xrpld that you will be emitting at most @count@ transactions during the course of this hook execution.
 * @param count The number of transactions you intend to emit from this  hook.
 * @return If a negaitve integer an error has occured
 */
int64_t etxn_reserve(uint32_t count);

int64_t float_compare(int64_t float1, int64_t float2, uint32_t mode);
int64_t float_divide(int64_t float1, int64_t float2);
int64_t float_int(int64_t float1, uint32_t decimal_places, uint32_t abs);
int64_t float_set(int32_t exponent, int64_t mantissa);
int64_t float_sum(int64_t float1, int64_t float2);
int64_t float_sto(uint32_t write_ptr, uint32_t write_len, uint32_t cread_ptr, uint32_t cread_len, uint32_t iread_ptr, uint32_t iread_len, int64_t float1, uint32_t field_code);
int64_t float_multiply(int64_t float1, int64_t float2);
int64_t float_negate(int64_t float1);

/**
 * Retrieve the account the hook is running on.
 * @param write_ptr A buffer of at least 20 bytes to write into.
 * @param write_len The length of that buffer
 * @return The number of bytes written into the buffer of a negative integer if an error occured.
 */
int64_t hook_account(uint32_t write_ptr, uint32_t write_len);

/**
 * Retrieve the current ledger sequence number
 */
int64_t ledger_seq(void);
int64_t ledger_last_hash(uint32_t write_ptr, uint32_t write_len);

/**
 * Retrieve a field from the originating transaction in its raw serialized form.
 * @param write_ptr A buffer to output the field into
 * @param write_len The length of the buffer.
 * @param field_if The field code of the field being requested
 * @return The number of bytes written to write_ptr or a negative integer if an error occured.
 */
int64_t otxn_field(uint32_t write_ptr, uint32_t write_len, uint32_t field_id);

/**
 * Retrieve the TXNID of the originating transaction.
 * @param write_ptr A buffer at least 32 bytes long
 * @param write_len The length of the buffer.
 * @return The number of bytes written into the buffer or a negative integer on failure.
 */
int64_t otxn_id(uint32_t write_ptr, uint32_t write_len, uint32_t flags);

int64_t otxn_slot(uint32_t slot);

/**
 * Retrieve the Transaction Type (e.g. ttPayment = 0) of the originating transaction.
 * @return The Transaction Type (tt-code)
 */
int64_t otxn_type(void);

/**
 * Read an r-address from the memory pointed to by read_ptr of length read_len and decode it to a 20 byte account id
 * and write to write_ptr
 * @param read_ptr The memory address of the r-address
 * @param read_len The byte length of the r-address
 * @param write_ptr The memory address of a suitable buffer to write the decoded account id into.
 * @param write_len The size of the write buffer.
 * @return On success 20 will be returned indicating the bytes written. On failure a negative integer is returned
 *         indicating what went wrong.
 */
int64_t util_accid(uint32_t write_ptr, uint32_t write_len, uint32_t read_ptr, uint32_t read_len);

int64_t slot(uint32_t write_ptr, uint32_t write_len, uint32_t slot);
int64_t slot_float(uint32_t slot);
int64_t slot_subfield(uint32_t parent_slot, uint32_t field_id, uint32_t new_slot);
int64_t slot_type(uint32_t slot, uint32_t flags);
int64_t slot_set(uint32_t read_ptr, uint32_t read_len, int32_t slot);

/**
 * Index into a xrpld serialized array and return the location and length of an index. Unlike sto_subfield this api
 * always returns the offset and length of the whole object at that index (not its payload.) Use SUB_OFFSET and
 * SUB_LENGTH macros to extract return value.
 * @param read_ptr The memory location of the stobject
 * @param read_len The length of the stobject
 * @param array_id The index requested
 * @return high-word (most significant 4 bytes excluding the most significant bit (MSB)) is the field offset relative
 *         to read_ptr and the low-word (least significant 4 bytes) is its length. MSB is sign bit, if set (negative)
 *         return value indicates error (typically error means could not find.)
 */
int64_t sto_subarray(uint32_t read_ptr, uint32_t read_len, uint32_t array_id);

/**
 * Index into a xrpld serialized object and return the location and length of a subfield. Except for Array subtypes
 * the offset and length refer to the **payload** of the subfield not the entire subfield. Use SUB_OFFSET and
 * SUB_LENGTH macros to extract return value.
 * @param read_ptr The memory location of the stobject
 * @param read_len The length of the stobject
 * @param field_id The Field Code of the subfield
 * @return high-word (most significant 4 bytes excluding the most significant bit (MSB)) is the field offset relative
 *         to read_ptr and the low-word (least significant 4 bytes) is its length. MSB is sign bit, if set (negative)
 *         return value indicates error (typically error means could not find.)
 */
int64_t sto_subfield(uint32_t read_ptr, uint32_t read_len, uint32_t field_id);

/**
 * Print some output to the trace log on xrpld. Any xrpld instance set to "trace" log level will see this.
 * @param read_ptr A buffer containing either data or text (in either utf8, or utf16le)
 * @param read_len The byte length of the data/text to send to the trace log
 * @param as_hex If 0 treat the read_ptr as pointing at a string of text, otherwise treat it as data and print hex
 * @return The number of bytes output or a negative integer if an error occured.
 */
int64_t trace(uint32_t mread_ptr, uint32_t mread_len, uint32_t dread_ptr, uint32_t dread_len, uint32_t as_hex);

/**
 * Print some output to the trace log on xrpld along with a decimal number. Any xrpld instance set to "trace" log
 * level will see this.
 * @param read_ptr A pointer to the string to output
 * @param read_len The length of the string to output
 * @param number Any integer you wish to display after the text
 * @return A negative value on error
 */
int64_t trace_num(uint32_t read_ptr, uint32_t read_len, int64_t number);

/**
 * Print some output to the trace log on xrpld along with a float in scientfic notation. Any xrpld instance set to "trace" log
 * level will see this.
 * @param read_ptr A pointer to the string to output
 * @param read_len The length of the string to output
 * @param number Any float in xfl format you wish to display after the text
 * @return A negative value on error
 */
int64_t trace_float(uint32_t mread_ptr, uint32_t mread_len, int64_t float1);

/**
 * Emit a transaction from this hook.
 * @param read_ptr Memory location of a buffer containing the fully formed binary transaction to emit.
 * @param read_len The length of the transaction.
 * @return A negative integer if the emission failed.
 */
int64_t emit(uint32_t write_ptr, uint32_t write_len, uint32_t read_ptr, uint32_t read_len);

int64_t util_keylet(uint32_t write_ptr, uint32_t write_len, uint32_t keylet_type, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f);

/**
 * In the hook's state key-value map, set the value for the key pointed at by kread_ptr.
 * @param read_ptr A buffer containing the data to store
 * @param read_len The length of the data
 * @param kread_ptr A buffer containing the key
 * @param kread_len The length of the key
 * @return The number of bytes stored or a negative integer if an error occured
 */
int64_t state_set(uint32_t read_ptr, uint32_t read_len, uint32_t kread_ptr, uint32_t kread_len);

/**
 * Retrieve a value from the hook's key-value map.
 * @param write_ptr A buffer to write the state value into
 * @param write_len The length of that buffer
 * @param kread_ptr A buffer to read the state key from
 * @param kread_len The length of that key
 * @return The number of bytes written or a negative integer if an error occured.
 */
int64_t state(uint32_t write_ptr, uint32_t write_len, uint32_t kread_ptr, uint32_t kread_len);

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
#define DIVISION_BY_ZERO -25     // API call would result in a division by zero, so API ended early.
#define CANT_RETURN_NEGATIVE -33 // An API would have returned a negative integer except that negative integers are reserved for error codes (i.e. what you are reading.)

#define INVALID_FLOAT -10024 // if the mantissa or exponent are outside normalized ranges

#define SLOT_TRANSACTION 0
#define SLOT_AMOUNT 23
#define SLOT_BALANCE 24
#define SLOT_LIMIT 25
#define SLOT_SEQUENCE 26

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
