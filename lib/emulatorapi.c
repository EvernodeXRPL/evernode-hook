#include "emulatorapi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define EMIT_DETAILS_LEN 105
#define TRANSACTION_HASH_LEN 32
#define STATE_KEY_LEN 32
#define MAX_STATE_VAL_LEN 128
#define MAX_READ_LEN 1024
#define TRUSTLINE_LEN 16
#define SEQUENCE_LEN 4

#define MANTISSA_OVERSIZED -26
#define EXPONENT_OVERSIZED -28
#define EXPONENT_UNDERSIZED -29

static int64_t const minMantissa = 1000000000000000ull;
static int64_t const maxMantissa = 9999999999999999ull;
static int32_t const minExponent = -96;
static int32_t const maxExponent = 80;

// Globals.
struct Transaction *txn;
uint8_t hook_accid[ACCOUNT_LEN];
int64_t slots_arr[255];
uint8_t sl_count = 0;

struct etxn_info etxn_inf = {0, 0};

// Local function definitions.
uint64_t get_mantissa(int64_t float1);
int32_t get_exponent(int64_t float1);
int is_negative(int64_t float1);
int64_t invert_sign(int64_t float1);
int write_stdout(const uint8_t *write_buf, const int write_len);
int get_trustlines(uint32_t address, uint32_t issuer, uint32_t currency, int64_t *balance_float, int64_t *limit_float);

/**
 * Read data from STDIN to the buffer.
 * @param read_buf Buffer to be read.
 * @param read_len Length of the buffer.
 * @return -1 if error, otherwise read length.
*/
int read_stdin(uint8_t *read_buf, const int read_len)
{
    // Read the response from STDIN.
    return read(STDIN_FILENO, read_buf, read_len);
}

/**
 * Write given buffer to the STDOUT.
 * Adds the data length as a prefix according to the message protocol.
 * @param write_buf Buffer to be sent.
 * @param write_len Length of the buffer.
 * @return -1 if error, otherwise write length.
*/
int write_stdout(const uint8_t *write_buf, const int write_len)
{
    const int outlen = 4 + write_len;
    char out[outlen];

    // Populate the data length as prefix.
    out[0] = write_len >> 24;
    out[1] = write_len >> 16;
    out[2] = write_len >> 8;
    out[3] = write_len;

    // Populate the data.
    memcpy(&out[4], write_buf, write_len);

    return (write(STDOUT_FILENO, out, outlen) == -1 || fflush(stdout) == -1) ? -1 : write_len;
}

int32_t _g(uint32_t id, uint32_t maxiter)
{
    return 0;
}

int64_t accept(uint32_t read_ptr, uint32_t read_len, int64_t error_code)
{
    trace(SBUF("Accept: "), read_ptr, read_len, 0);
    exit(0);
    return 0;
}

int64_t etxn_fee_base(uint32_t tx_byte_count)
{
    // Minimum emission fee.
    return 12;
}
int64_t etxn_details(uint32_t write_ptr, uint32_t write_len)
{
    return 0;
}
int64_t etxn_reserve(uint32_t count)
{
    // Acording to the xrpl hook specs,
    // The hook already called this function earlier, return ALREADY_SET.
    if (etxn_inf.etxn_reserved > 1)
        return ALREADY_SET;

    etxn_inf.etxn_reserved = 1;
    etxn_inf.etxn_reserves += count;

    return etxn_inf.etxn_reserves;
}

int64_t float_compare(int64_t float1, int64_t float2, uint32_t mode)
{
    const int64_t sum = float_sum(float1, invert_sign(float2));

    if (mode == COMPARE_LESS)
        return (sum != 0 && is_negative(sum)) ? 1 : 0;
    else if (mode == COMPARE_EQUAL)
        return sum == 0 ? 1 : 0;
    else if (mode == COMPARE_GREATER)
        return (sum != 0 && !is_negative(sum)) ? 1 : 0;
    else if (mode == (COMPARE_LESS | COMPARE_GREATER)) // Not equal.
        return sum != 0 ? 1 : 0;
    else if (mode == (COMPARE_LESS | COMPARE_EQUAL)) // Less than or equal to.
        return (sum == 0 || is_negative(sum)) ? 1 : 0;
    else if (mode == (COMPARE_GREATER | COMPARE_EQUAL)) // Greater than or equal.
        return (sum == 0 || !is_negative(sum)) ? 1 : 0;
    else
        return INVALID_ARGUMENT;
}

int64_t float_divide(int64_t float1, int64_t float2)
{
    const uint64_t mantissa_1 = get_mantissa(float1);
    const uint64_t mantissa_2 = get_mantissa(float2);

    const int32_t exponent_1 = get_exponent(float1);
    const int32_t exponent_2 = get_exponent(float2);

    if (mantissa_2 == 0)
        return DIVISION_BY_ZERO;

    const uint64_t ans_mantissa = mantissa_1 / mantissa_2;
    const int64_t ans_exponent = exponent_1 - exponent_2;
    int64_t ans = float_set(ans_exponent, ans_mantissa);

    if (is_negative(float1) ^ is_negative(float2))
        ans = float_negate(ans);

    return ans;
}
int64_t float_int(int64_t float1, uint32_t decimal_places, uint32_t abs)
{
    if (decimal_places > 15)
        return INVALID_ARGUMENT;
    else if (decimal_places == 0)
        return 0;

    const uint64_t mantissa = get_mantissa(float1);
    const int32_t exponent = get_exponent(float1) + decimal_places;

    int64_t ret = mantissa;
    if (exponent < 0)
    {
        for (size_t i = 0; i < (exponent * -1); i++)
            ret = ret / 10;
    }
    else
    {
        for (size_t i = 0; i < exponent; i++)
            ret = ret * 10;
    }

    if (is_negative(float1) && !abs)
        return CANT_RETURN_NEGATIVE;

    return ret;
}

int32_t get_exponent(int64_t float1)
{
    if (float1 < 0)
        return INVALID_FLOAT;
    if (float1 == 0)
        return 0;
    if (float1 < 0)
        return INVALID_FLOAT;

    uint64_t float_in = (uint64_t)float1;
    float_in >>= 54U;
    float_in &= 0xFFU;
    return ((int32_t)float_in) - 97;
}

uint64_t get_mantissa(int64_t float1)
{
    if (float1 < 0)
        return INVALID_FLOAT;
    if (float1 == 0)
        return 0;
    if (float1 < 0)
        return INVALID_FLOAT;
    float1 -= ((((uint64_t)float1) >> 54U) << 54U);
    return float1;
}
int is_negative(int64_t float1)
{
    return (float1 >> 62U) != 0 ? 0 : 1;
}

int64_t invert_sign(int64_t float1)
{
    return float1 ^ (1ULL << 62U);
}

int64_t set_sign(int64_t float1, int set_negative)
{
    int neg = is_negative(float1);
    if ((neg == 1 && set_negative) || (neg == 0 && set_negative == 0))
        return float1;

    return invert_sign(float1);
}

int64_t set_mantissa(int64_t float1, uint64_t mantissa)
{
    if (mantissa > maxMantissa)
        return MANTISSA_OVERSIZED;
    return float1 - get_mantissa(float1) + mantissa;
}

int64_t set_exponent(int64_t float1, int32_t exponent)
{
    if (exponent > maxExponent)
        return EXPONENT_OVERSIZED;
    if (exponent < minExponent)
        return EXPONENT_UNDERSIZED;

    uint64_t exp = (exponent + 97);
    exp <<= 54U;
    float1 &= ~(0xFFLL << 54);
    float1 += (int64_t)exp;
    return float1;
}

int64_t make_float(int64_t mantissa, int32_t exponent)
{
    if (mantissa == 0)
        return 0;
    if (mantissa > maxMantissa)
        return MANTISSA_OVERSIZED;
    if (exponent > maxExponent)
        return EXPONENT_OVERSIZED;
    if (exponent < minExponent)
        return EXPONENT_UNDERSIZED;
    int neg = mantissa < 0 ? 1 : 0;
    if (neg == 1)
        mantissa *= -1LL;
    int64_t out = 0;
    out = set_mantissa(out, mantissa);
    out = set_exponent(out, exponent);
    out = set_sign(out, neg);
    return out;
}

int64_t float_set(int32_t exponent, int64_t mantissa)
{
    if (mantissa == 0)
        return 0;

    int neg = mantissa < 0 ? 1 : 0;
    if (neg == 1)
        mantissa *= -1LL;

    // normalize
    while (mantissa < minMantissa)
    {
        mantissa *= 10;
        exponent--;
        if (exponent < minExponent)
            return INVALID_FLOAT; //underflow
    }
    while (mantissa > maxMantissa)
    {
        mantissa /= 10;
        exponent++;
        if (exponent > maxExponent)
            return INVALID_FLOAT; //overflow
    }

    return make_float((neg ? -1LL : 1LL) * mantissa, exponent);
}

int64_t float_sum(int64_t float1, int64_t float2)
{
    uint64_t mantissa_1 = get_mantissa(float1);
    int32_t exponent_1 = get_exponent(float1);

    uint64_t mantissa_2 = get_mantissa(float2);
    int32_t exponent_2 = get_exponent(float2);

    if (exponent_1 > exponent_2)
    {
        for (size_t i = 0; i < (exponent_1 - exponent_2); i++)
        {
            mantissa_1 *= 10;
            exponent_1--;
        }
    }
    else if (exponent_1 < exponent_2)
    {
        for (size_t i = 0; i < (exponent_2 - exponent_1); i++)
        {
            mantissa_2 *= 10;
            exponent_2--;
        }
    }

    const int64_t num_1 = mantissa_1 * (is_negative(float1) ? -1 : 1);
    const int64_t num_2 = mantissa_2 * (is_negative(float2) ? -1 : 1);
    return float_set(exponent_1, num_1 + num_2);
}

int64_t float_sto(uint32_t write_ptr, uint32_t write_len, uint32_t cread_ptr, uint32_t cread_len, uint32_t iread_ptr, uint32_t iread_len, int64_t float1, uint32_t field_code)
{
    if (field_code != -1)
    {
        trace(SBUF("Error: Only field code == -1 case is implemented."), 0, 0, 0);
        return -1;
    }
    uint8_t amount_buf[8];
    uint64_t one = 1;
    int64_t to_write = (float1 | (one << 63));
    INT64_TO_BUF(amount_buf, to_write);
    memcpy((uint32_t *)write_ptr, amount_buf, sizeof(amount_buf));

    if (field_code > 0)
    {
        if (cread_len == 20)
            memcpy((uint32_t *)(write_ptr + 8), cread_ptr, cread_len);

        if (iread_len == 20)
            memcpy((uint32_t *)(write_ptr + 28), iread_ptr, iread_len);
    }
    return 0;
}

int64_t float_multiply(int64_t float1, int64_t float2)
{
    uint64_t mantissa_1 = get_mantissa(float1);
    int32_t exponent_1 = get_exponent(float1);
    // Reduce trailing zeros to prevent overflow when multiplying.
    while (mantissa_1 % 10 == 0)
    {
        mantissa_1 /= 10;
        exponent_1++;
    }

    uint64_t mantissa_2 = get_mantissa(float2);
    int32_t exponent_2 = get_exponent(float2);
    // Reduce trailing zeros to prevent overflow when multiplying.
    while (mantissa_2 % 10 == 0)
    {
        mantissa_2 /= 10;
        exponent_2++;
    }

    uint64_t ans_mantissa = mantissa_1 * mantissa_2;
    int64_t ans_exponent = exponent_1 + exponent_2;
    int64_t answer_float = float_set(ans_exponent, ans_mantissa);

    if (is_negative(float1) ^ is_negative(float2))
        answer_float = invert_sign(answer_float);
    return answer_float;
}

int64_t float_negate(int64_t float1)
{
    return invert_sign(float1);
}

int64_t hook_account(uint32_t write_ptr, uint32_t write_len)
{
    memcpy((uint32_t *)write_ptr, hook_accid, sizeof(hook_accid));
    return sizeof(hook_accid);
}

int64_t ledger_seq(void)
{
    return txn->ledger_index;
}

int64_t ledger_last_hash(uint32_t write_ptr, uint32_t write_len)
{
    memcpy((uint32_t *)write_ptr, txn->ledger_hash, HASH_SIZE_1);
    return HASH_SIZE_1;
}

int64_t otxn_field(uint32_t write_ptr, uint32_t write_len, uint32_t field_id)
{
    int ret = -1;
    switch (field_id)
    {
    case sfAccount:
    {
        memcpy((uint32_t *)write_ptr, txn->account, sizeof(txn->account));
        ret = sizeof(txn->account);
        break;
    }
    case sfMemos:
    {
        int memo_data_len = sizeof(struct Memo) * txn->memos.memo_count;
        memcpy((uint32_t *)write_ptr, txn->memos.list, memo_data_len);
        ret = memo_data_len;
        break;
    }

    default:
        trace(SBUF("Error: Only supports sfAccount and sfMemos as field_id"), 0, 0, 0);
        break;
    }
    return ret;
}

int64_t otxn_id(uint32_t write_ptr, uint32_t write_len, uint32_t flags)
{
    memcpy((uint32_t *)write_ptr, txn->ledger_hash, HASH_SIZE_1);
    return HASH_SIZE_1;
}

int64_t otxn_slot(uint32_t slot)
{
    uint8_t *ptr = (uint8_t *)malloc(4 + sizeof(struct Transaction));
    UINT32_TO_BUF(ptr, SLOT_TRANSACTION);
    memcpy(ptr + 4, (uint8_t *)txn, sizeof(struct Transaction));
    // Keep track of slots so we can cleanup before the program exits.
    slots_arr[sl_count++] = ptr;
}

int64_t otxn_type(void)
{
    // Only the payment transactions are handled in our scenario.
    return ttPAYMENT;
}

int64_t util_accid(uint32_t write_ptr, uint32_t write_len, uint32_t read_ptr, uint32_t read_len)
{
    if (write_len < 20)
        return TOO_SMALL;

    if (read_len > 35)
        return TOO_BIG;

    const int len = 1 + strlen((char *)read_ptr);
    char buf[len];

    // Populate the type header.
    buf[0] = ACCID;

    sprintf(&buf[1], "%s", (char *)read_ptr);
    const int write_res = write_stdout(buf, len);
    if (write_res < 0)
        return -1;

    // Read return code of the response.
    int8_t ret;
    if (read_stdin(&ret, 1) < 0 || ret < 0)
        return -1;

    return read_stdin(write_ptr, write_len);
}

int64_t slot(uint32_t write_ptr, uint32_t write_len, uint32_t slot)
{
    uint8_t *slot_ptr = (uint32_t *)slot;
    uint32_t type = UINT32_FROM_BUF(slot_ptr);

    if (type == SLOT_AMOUNT)
    {
        struct Amount *amt = (struct Amount *)(slot_ptr + 4);
        if (amt->is_xrp)
        {
            if (write_len < 8)
                return TOO_SMALL;

            uint8_t amount_buf[8];
            INT64_TO_BUF(amount_buf, amt->xrp.amount);
            memcpy((uint32_t *)write_ptr, amount_buf, sizeof(amount_buf));
            return sizeof(amount_buf);
        }
        else
        {
            if (write_len < 48)
                return TOO_SMALL;

            uint8_t amount_buf[8];
            INT64_TO_BUF(amount_buf, amt->iou.amount);
            memcpy((uint32_t *)write_ptr, amount_buf, 8);
            memcpy((uint32_t *)(write_ptr + 8), amt->iou.currency, 20);
            memcpy((uint32_t *)(write_ptr + 28), amt->iou.issuer, 20);
            return 48;
        }
    }
    else if (type == SLOT_SEQUENCE)
    {
        memcpy((uint32_t *)write_ptr, (slot_ptr + 4), 4);
        return 4;
    }
}

int64_t slot_float(uint32_t slot)
{
    uint8_t *slot_ptr = (uint32_t *)slot;
    uint32_t type = UINT32_FROM_BUF(slot_ptr);

    if (type == SLOT_AMOUNT)
    {
        struct Amount *amt = (struct Amount *)(slot_ptr + 4);
        if (amt->is_xrp)
            return amt->xrp.amount;
        else
            return amt->iou.amount;
    }
    else if (type == SLOT_BALANCE)
    {
        return INT64_FROM_BUF(slot_ptr + 4);
    }
    else if (type == SLOT_LIMIT)
    {
        return INT64_FROM_BUF(slot_ptr + 4);
    }
}

int64_t slot_subfield(uint32_t parent_slot, uint32_t field_id, uint32_t new_slot)
{
    uint8_t *slot_ptr = (uint32_t *)parent_slot;
    uint32_t type = UINT32_FROM_BUF(slot_ptr);

    if (type == SLOT_TRANSACTION)
    {
        struct Transaction *tx = (struct Transaction *)(slot_ptr + 4);

        if (field_id == sfAmount)
        {
            int len = 4 + sizeof(struct Amount);
            uint8_t slot_buf[len];
            UINT32_TO_BUF(slot_buf, SLOT_AMOUNT);
            memcpy(slot_buf + 4, (uint8_t *)tx, sizeof(struct Amount));
            return slot_set(SBUF(slot_buf), new_slot);
        }
    }
    else if (type == KEYLET_LINE)
    {
        if (field_id == sfBalance)
        {
            int len = 8;
            uint8_t slot_buf[len];
            UINT32_TO_BUF(slot_buf, SLOT_BALANCE);
            memcpy(slot_buf + 4, (slot_ptr + 4), 8);
            return slot_set(SBUF(slot_buf), new_slot);
        }
        else if (field_id == sfLimitAmount)
        {
            int len = 8;
            uint8_t slot_buf[len];
            UINT32_TO_BUF(slot_buf, SLOT_LIMIT);
            memcpy(slot_buf + 4, (slot_ptr + 12), 8);
            return slot_set(SBUF(slot_buf), new_slot);
        }
    }
    else if (type == KEYLET_ACCOUNT)
    {
        if (field_id == sfSequence)
        {
            int len = 8;
            uint8_t slot_buf[len];
            UINT32_TO_BUF(slot_buf, SLOT_SEQUENCE);
            memcpy(slot_buf + 4, (slot_ptr + 4), 4);
            return slot_set(SBUF(slot_buf), new_slot);
        }
    }

    return -1;
}

int64_t slot_type(uint32_t slot, uint32_t flags)
{
    uint8_t *slot_ptr = (uint8_t *)slot;
    uint32_t type = UINT16_FROM_BUF(slot_ptr);
    if (type == SLOT_AMOUNT)
    {
        struct Amount *amt = (struct Amount *)(slot_ptr + 4);
        return amt->is_xrp;
    }
    return -1;
}
int64_t slot_set(uint32_t read_ptr, uint32_t read_len, int32_t slot)
{
    uint8_t *ptr = (uint8_t *)malloc(read_len);
    memcpy(ptr, (uint32_t *)read_ptr, read_len);
    // Keep track of slots so we can cleanup before the program exits.
    slots_arr[sl_count++] = ptr;
    return ptr;
}

int64_t sto_subarray(uint32_t read_ptr, uint32_t read_len, uint32_t array_id)
{
    trace(SBUF("Currently support only memos..."), 0, 0, 0);
    struct Memo *list = (struct Memo *)read_ptr;
    int64_t offset = sizeof(struct Memo) * array_id;
    int64_t len = sizeof(struct Memo);
    int64_t ans = offset << 32;
    ans = ans | len;
    return ans;
}

int64_t sto_subfield(uint32_t read_ptr, uint32_t read_len, uint32_t field_id)
{
    if (field_id == sfMemo)
    {
        int64_t ans = read_ptr << 32;
        ans = ans | read_len;
        return ans;
    }
    else if (field_id == sfMemoType)
    {
        struct Memo *memo = (struct Memo *)read_ptr;
        int64_t ans = ((int64_t)memo->type - read_ptr) << 32;
        ans = ans | (int64_t)memo->type_len;
        return ans;
    }
    else if (field_id == sfMemoData)
    {
        struct Memo *memo = (struct Memo *)read_ptr;
        int64_t ans = ((int64_t)memo->data - read_ptr) << 32;
        ans = ans | (int64_t)memo->data_len;
        return ans;
    }
    else if (field_id == sfMemoFormat)
    {
        struct Memo *memo = (struct Memo *)read_ptr;
        int64_t ans = ((int64_t)memo->format - read_ptr) << 32;
        ans = ans | (int64_t)memo->format_len;
        return ans;
    }
    else
    {
        trace(SBUF("Error: Currently support only memo related fields..."), 0, 0, 0);
        return -1;
    }
}

/**
 * Send the trace request.
 * Appdends the trace type header.
 * @param trace Data buffer to be traced.
 * @return -1 if error, otherwise write length.
*/
int trace_out(const uint8_t *trace)
{
    const int len = 1 + strlen(trace);
    char buf[len];

    // Populate the type header.
    buf[0] = TRACE;

    sprintf(&buf[1], "%s", trace);
    return write_stdout(buf, len);
}

int64_t trace(uint32_t mread_ptr, uint32_t mread_len, uint32_t dread_ptr, uint32_t dread_len, uint32_t as_hex)
{
    if (as_hex == 1)
    {
        // If hex (byte => 2 ascci characters) allocate a buffer and populate the hex characters in the loop.
        char out[mread_len + (dread_len * 2) + 1];
        sprintf(out, "%*.*s ", 0, mread_len, (char *)mread_ptr);
        for (int i = 0; i < dread_len; i++)
            sprintf(&out[mread_len + (i * 2)], "%02X", *(uint8_t *)(dread_ptr + i));
        trace_out(out);
    }
    else
    {
        char out[mread_len + dread_len + 1];
        sprintf(out, "%*.*s %*.*s", 0, mread_len, (char *)mread_ptr, 0, dread_len, (char *)dread_ptr);
        trace_out(out);
    }
    return 0;
}

int64_t trace_num(uint32_t read_ptr, uint32_t read_len, int64_t number)
{
    char out[500];
    sprintf(out, "%*.*s %lld", 0, read_len, (char *)read_ptr, number);
    trace_out(out);
    return 0;
}

int64_t trace_float(uint32_t mread_ptr, uint32_t mread_len, int64_t float1)
{
    const uint64_t mantissa = get_mantissa(float1);
    char out[500];
    sprintf(out, (mantissa != 0 && is_negative(float1)) ? "%*.*s Float -%lld*10^(%d)" : "%*.*s Float %lld*10^(%d)\n",
            0, mread_len, (char *)mread_ptr, mantissa, get_exponent(float1));
    trace_out(out);
    return 0;
}

int64_t emit(uint32_t write_ptr, uint32_t write_len, uint32_t read_ptr, uint32_t read_len)
{
    // Acording to the xrpl hook specs,
    // If the hook didn't call etxn_reserve, return PREREQUISITE_NOT_MET.
    // If the number of promised transactions are not enough, return TOO_MANY_EMITTED_TXN.
    if (etxn_inf.etxn_reserved == 0)
        return PREREQUISITE_NOT_MET;
    else if (etxn_inf.etxn_reserves == 0)
        return TOO_MANY_EMITTED_TXN;

    etxn_inf.etxn_reserves--;

    // Send the emit request.
    // Note: when transaction is prepared in hook logic, it populats additional emit details at the end.
    // Skip the emit deatils since those aren't recognized in the xrpl lib.
    const int buflen = 1 + read_len - EMIT_DETAILS_LEN;
    uint8_t buf[buflen];
    // Populate the type header.
    buf[0] = EMIT;
    memcpy(&buf[1], (uint32_t *)read_ptr, buflen - 1);
    write_stdout(buf, buflen);

    // Read the response from STDIN.
    uint8_t data_buf[TRANSACTION_HASH_LEN + 1];
    const int data_len = read_stdin(data_buf, sizeof(data_buf));
    const int ret = (int8_t)*data_buf;
    const uint8_t *res = &data_buf[1];

    // If result code is less than 0 return EMISSION_FAILURE.
    if (ret < 0 || (data_len - 1) != TRANSACTION_HASH_LEN)
    {
        write_ptr = 0;
        return EMISSION_FAILURE;
    }

    // Populate the received transaction hash to the write pointer.
    memcpy((uint32_t *)write_ptr, res, TRANSACTION_HASH_LEN);
    return 0;
}

/**
 * Get trustlines of the running account.
 * @param address Source address of the trustlines.
 * @param issuer Issuer of the trustline currency.
 * @param currency Issued currency.
 * @param balance_float Float balance to be populated.
 * @param limit_float Limit float to be populated.
*/
int get_trustlines(uint32_t address, uint32_t issuer, uint32_t currency, int64_t *balance_float, int64_t *limit_float)
{
    // Send the emit request.
    const int len = 44;
    uint8_t buf[len];
    // Populate the type header.
    buf[0] = KEYLET;
    // Populate the address, issuer and currency.
    memcpy(&buf[1], (uint32_t *)address, 20);
    memcpy(&buf[21], (uint32_t *)issuer, 20);
    memcpy(&buf[41], (uint32_t *)(currency + 12), 3);
    write_stdout(buf, len);

    // Read the response from STDIN.
    uint8_t data_buf[TRUSTLINE_LEN + 1];
    const int data_len = read_stdin(data_buf, sizeof(data_buf));
    const int ret = (int8_t)*data_buf;
    const uint8_t *res = &data_buf[1];

    if (ret < 0 || ((data_len - 1) != TRUSTLINE_LEN))
    {
        *balance_float = 0;
        *limit_float = 0;
        // Return the error code according to the return code.
        if (ret == RES_NOT_FOUND)
            return DOESNT_EXIST;

        return -1;
    }

    *balance_float = INT64_FROM_BUF(res);
    *limit_float = INT64_FROM_BUF((res + 8));
    return 0;
}

/**
 * Get sequence number of the given account.
 * @param address Source address of the sequence.
 * @param sequence Limit float to be populated.
*/
int get_sequence(uint32_t address, uint32_t *sequence)
{
    // Send the sequence request.
    const int len = 21;
    uint8_t buf[len];
    // Populate the type header.
    buf[0] = SEQUENCE;
    // Populate the address.
    memcpy(&buf[1], (uint32_t *)address, 20);
    write_stdout(buf, len);

    // Read the response from STDIN.
    uint8_t data_buf[TRUSTLINE_LEN + 1];
    const int data_len = read_stdin(data_buf, sizeof(data_buf));
    const int ret = (int8_t)*data_buf;
    const uint8_t *res = &data_buf[1];

    if (ret < 0 || ((data_len - 1) != SEQUENCE_LEN))
    {
        *sequence = 0;
        return -1;
    }

    *sequence = UINT32_FROM_BUF(res);
    return 0;
}

int64_t util_keylet(uint32_t write_ptr, uint32_t write_len, uint32_t keylet_type, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f)
{
    if (keylet_type != KEYLET_LINE && keylet_type != KEYLET_ACCOUNT)
    {
        trace(SBUF("Error: Only KEYLET_LINE and KEYLET_ACCOUNT is supported."), 0, 0, 0);
        return -1;
    }
    else if (keylet_type == KEYLET_LINE && (b != 20 | d != 20 | f != 20))
    {
        trace(SBUF("Error: High account, low account and currency length should be 20."), 0, 0, 0);
        return -1;
    }
    else if (keylet_type == KEYLET_ACCOUNT && (b != 20))
    {
        trace(SBUF("Error: Account length should be 20."), 0, 0, 0);
        return -1;
    }

    if (keylet_type == KEYLET_LINE)
    {
        if (b != 20 | d != 20 | f != 20)
        {
            trace(SBUF("Error: High account, low account and currency length should be 20."), 0, 0, 0);
            return -1;
        }

        int64_t balance, limit;
        int trustline_res = get_trustlines(a, c, e, &balance, &limit);
        if (trustline_res < 0)
            return trustline_res;

        uint8_t buf[20];
        UINT32_TO_BUF((uint32_t *)(write_ptr), keylet_type);
        INT64_TO_BUF((uint32_t *)(write_ptr + 4), balance);
        INT64_TO_BUF((uint32_t *)(write_ptr + 12), limit);
        memset((uint32_t *)(write_ptr + 20), 0, write_len - 20);

        return write_len;
    }
    else if (keylet_type == KEYLET_ACCOUNT)
    {
        if (b != 20)
        {
            trace(SBUF("Error: Account length should be 20."), 0, 0, 0);
            return -1;
        }

        uint32_t sequence;
        int sequence_res = get_sequence(a, &sequence);
        if (sequence_res < 0)
            return sequence_res;

        uint8_t buf[8];
        UINT32_TO_BUF((uint32_t *)(write_ptr), keylet_type);
        UINT32_TO_BUF((uint32_t *)(write_ptr + 4), sequence);
        memset((uint32_t *)(write_ptr + 8), 0, write_len - 8);

        return write_len;
    }
    else
    {
        trace(SBUF("Error: Only KEYLET_LINE and KEYLET_ACCOUNT is supported."), 0, 0, 0);
        return -1;
    }
}

int64_t state_set(uint32_t read_ptr, uint32_t read_len, uint32_t kread_ptr, uint32_t kread_len)
{
    // Acording to the xrpl hook specs,
    // If kread_len is greater than max key len or read len is greater than max data len, return TOO_BIG.
    // If kread_len is 0, return TOO_SMALL.
    if (kread_len > STATE_KEY_LEN || read_len > MAX_STATE_VAL_LEN)
        return TOO_BIG;
    else if (kread_len == 0)
        return TOO_SMALL;

    // Send the state set request.
    const int len = 1 + STATE_KEY_LEN + read_len;
    uint8_t buf[len];
    // Populate the type header.
    buf[0] = STATE_SET;
    // Populate the state data.
    memcpy(&buf[1], (uint32_t *)kread_ptr, kread_len);
    // If kread_len is less than STATE_KEY_LEN, populate o's to the rest.
    if (kread_len < STATE_KEY_LEN)
        memset(&buf[1 + kread_len], 0, STATE_KEY_LEN - kread_len);
    // Populate the data to rest.
    memcpy(&buf[1 + STATE_KEY_LEN], (uint32_t *)read_ptr, read_len);
    write_stdout(buf, len);

    // Read the response from STDIN.
    int8_t ret;
    const int data_len = read_stdin(&ret, 1);

    // Return the error code according to the return code.
    if (ret == RET_OVERFLOW)
        return TOO_BIG;
    else if (ret == RET_UNDERFLOW)
        return TOO_SMALL;
    else if (ret < -1)
        return -1;

    return 0;
}

int64_t state(uint32_t write_ptr, uint32_t write_len, uint32_t kread_ptr, uint32_t kread_len)
{
    // Acording to the xrpl hook specs,
    // If kread_len is greater than max key len, return TOO_BIG.
    if (kread_len > STATE_KEY_LEN)
        return TOO_BIG;

    // Send the state set request.
    const int len = 1 + kread_len;
    uint8_t buf[len];
    // Populate the type header.
    buf[0] = STATE_GET;
    // Populate the state key.
    memcpy(&buf[1], (uint32_t *)kread_ptr, kread_len);
    write_stdout(buf, len);

    // Read the response from STDIN.
    uint8_t data_buf[MAX_STATE_VAL_LEN + 1];
    const int data_len = read_stdin(data_buf, sizeof(data_buf));
    const int ret = (int8_t)*data_buf;
    const uint8_t *res = &data_buf[1];

    if (ret < 0)
    {
        write_ptr = 0;
        // Return the error code according to the return code.
        if (ret == RES_NOT_FOUND)
            return DOESNT_EXIST;
        else if (ret == RET_OVERFLOW)
            return TOO_BIG;
        else if (ret == RET_UNDERFLOW)
            return TOO_SMALL;

        return -1;
    }
    else if ((data_len - 1) > write_len)
    {
        // If the output buffer was too small to store the Hook State data return TOO_SMALL.
        write_ptr = 0;
        return TOO_SMALL;
    }
    else if ((data_len - 1) > 0)
        memcpy((uint32_t *)write_ptr, res, (data_len - 1)); // Populate only if res_len > 0;
    else
        write_ptr = 0;

    return 0;
}

int64_t rollback(uint32_t read_ptr, uint32_t read_len, int64_t error_code)
{
    trace(SBUF("Rollback: "), read_ptr, read_len, 0);
    exit(-1);
    return 0;
}
