#include "sim.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MANTISSA_OVERSIZED -26
#define EXPONENT_OVERSIZED -28
#define EXPONENT_UNDERSIZED -29

static int64_t const minMantissa = 1000000000000000ull;
static int64_t const maxMantissa = 9999999999999999ull;
static int32_t const minExponent = -96;
static int32_t const maxExponent = 80;

struct Transaction *txn;
uint8_t hook_accid[ACCOUNT_LEN];

int32_t _g(uint32_t id, uint32_t maxiter)
{
    return 0;
}

int64_t accept(uint32_t read_ptr, uint32_t read_len, int64_t error_code)
{
    printf("accept code: %d\n", error_code);
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
    return 0;
}

int64_t float_compare(int64_t float1, int64_t float2, uint32_t mode)
{
    uint64_t mantissa_1 = get_mantissa(float1);
    int64_t exponent_1 = get_exponent(float1);

    uint64_t mantissa_2 = get_mantissa(float2);
    int64_t exponent_2 = get_exponent(float2);

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

    const int neg_1 = is_negative(float1);
    const int neg_2 = is_negative(float2);

    if (mode == COMPARE_LESS)
    {
        if (neg_1 || neg_2)
        {
            if (neg_1 && neg_2)
                return (mantissa_1 > mantissa_2) ? 1 : 0;

            return (neg_1) ? 1 : 0;
        }
        else
            return mantissa_1 < mantissa_2 ? 1 : 0;
    }
    else if (mode == COMPARE_EQUAL)
    {
        if (neg_1 || neg_2)
        {
            if (neg_1 && neg_2)
                return (mantissa_1 == mantissa_2) ? 1 : 0;

            return 0;
        }
        else
            return mantissa_1 == mantissa_2 ? 1 : 0;
    }
    else if (mode == COMPARE_GREATER)
    {
        if (neg_1 || neg_2)
        {
            if (neg_1 && neg_2)
                return (mantissa_1 < mantissa_2) ? 1 : 0;

            return (neg_1) ? 0 : 1;
        }
        else
            return mantissa_1 > mantissa_2 ? 1 : 0;
    }
    else if (mode == (COMPARE_LESS | COMPARE_GREATER)) // Not equal.
    {
        if (neg_1 || neg_2)
        {
            if (neg_1 && neg_2)
                return (mantissa_1 != mantissa_2) ? 1 : 0;

            return 1;
        }
        else
            return mantissa_1 != mantissa_2 ? 1 : 0;
    }
    else if (mode == (COMPARE_LESS | COMPARE_EQUAL)) // Less than or equal to.
    {
        if (neg_1 || neg_2)
        {
            if (neg_1 && neg_2)
                return (mantissa_1 >= mantissa_2) ? 1 : 0;

            return (neg_1) ? 1 : 0;
        }
        else
            return mantissa_1 <= mantissa_2 ? 1 : 0;
    }
    else if (mode == (COMPARE_GREATER | COMPARE_EQUAL)) // Greater than or equal.
    {
        if (neg_1 || neg_2)
        {
            if (neg_1 && neg_2)
                return (mantissa_1 <= mantissa_2) ? 1 : 0;

            return (neg_1) ? 0 : 1;
        }
        else
            return mantissa_1 >= mantissa_2 ? 1 : 0;
    }
    return 0;
}

int64_t float_divide(int64_t float1, int64_t float2)
{
    uint64_t mentissa_1 = get_mantissa(float1);
    uint64_t mentissa_2 = get_mantissa(float2);

    int64_t exponent_1 = get_exponent(float1);
    int64_t exponent_2 = get_exponent(float2);

    if (mentissa_1 == 0 || mentissa_2 == 0)
    {
        return 0;
    }

    return 0;
}
int64_t float_int(int64_t float1, uint32_t decimal_places, uint32_t abs)
{
    if (decimal_places > 15)
        return INVALID_ARGUMENT;
    else if (decimal_places == 0)
        return 0;

    int64_t ret = 0;

    uint64_t mantissa = get_mantissa(float1);
    printf("mantissa: %lld\n", mantissa);
    int64_t exponent = get_exponent(float1);
    printf("exponent: %lld\n", exponent);

    exponent = exponent + decimal_places;

    ret = mantissa;
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

    int neg = is_negative(float1);
    printf("neg: %d\n", neg);
    printf("abs: %d\n", abs);
    if (neg && !abs)
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
    int64_t exponent_1 = get_exponent(float1);

    uint64_t mantissa_2 = get_mantissa(float2);
    int64_t exponent_2 = get_exponent(float2);

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

    const int neg_1 = is_negative(float1);
    const int neg_2 = is_negative(float2);

    uint64_t mantissa_sum = 0;
    int make_neg = 0;
    if (neg_1 && neg_2)
    {
        mantissa_sum = mantissa_1 + mantissa_2;
        make_neg = 1;
    }
    else if (neg_1 || neg_2)
    {
        if (mantissa_1 > mantissa_2)
        {
            mantissa_sum = mantissa_1 - mantissa_2;
            make_neg = neg_1;
        }
        else
        {
            mantissa_sum = mantissa_2 - mantissa_1;
            make_neg = neg_2;
        }
    }
    else
    {
        mantissa_sum = mantissa_1 + mantissa_2;
        make_neg = 0;
    }

    const int64_t exponent_sum = exponent_1;

    int64_t answer = float_set(exponent_sum, mantissa_sum);
    if (make_neg)
        answer = set_sign(answer, 1);

    return answer;
}
int64_t float_sto(uint32_t write_ptr, uint32_t write_len, uint32_t cread_ptr, uint32_t cread_len, uint32_t iread_ptr, uint32_t iread_len, int64_t float1, uint32_t field_code)
{
    return 0;
}
int64_t float_multiply(int64_t float1, int64_t float2)
{
    uint64_t mantissa_1 = get_mantissa(float1);
    int64_t exponent_1 = get_exponent(float1);
    while (mantissa_1 % 10 == 0)
    {
        mantissa_1 /= 10;
        exponent_1++;
    }

    uint64_t mantissa_2 = get_mantissa(float2);
    int64_t exponent_2 = get_exponent(float2);
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

void print_bytes(char *heading, uint8_t *write_ptr, uint32_t write_len)
{
    // uint32_t * ptr = &write_ptr;
    printf("%s: ", heading);
    for (int i = 0; i < write_len; i++)
    {
        printf("%02hhX", write_ptr[i]);
    }
    printf("\n");
}

int64_t hook_account(uint32_t write_ptr, uint32_t write_len)
{
    memcpy(write_ptr, hook_accid, sizeof(hook_accid));
    return sizeof(hook_accid);
}
// May be will need to hardcode the address. Since this is used to detect incomming or outgoing transactions.

// Not sure.
int64_t ledger_seq(void)
{
    return txn->ledger_index;
}
int64_t ledger_last_hash(uint32_t write_ptr, uint32_t write_len)
{
    memcpy(write_ptr, txn->ledger_hash, write_len);
    return 0;
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
    if (slot == 0)
    {
        return txn;
    }
    return 0;
}
int64_t otxn_type(void)
{
    return ttPAYMENT;
}
// 1. Receive transaction as a string format and load it to a struct.

int64_t util_accid(uint32_t write_ptr, uint32_t write_len, uint32_t read_ptr, uint32_t read_len)
{
    return 0;
}

int64_t slot(uint32_t write_ptr, uint32_t write_len, uint32_t slot)
{
    struct Amount *amt = (struct Amount *)slot;
    // <val><currency><issuer>
    if (amt->is_xrp)
    {
        if (write_len < 8)
            return TOO_SMALL;

        uint8_t amount_buf[8];
        INT64_TO_BUF(amount_buf, amt->xrp.amount);
        memcpy(write_ptr, amount_buf, sizeof(amount_buf));
        return sizeof(amount_buf);
    }
    else
    {
        if (write_len < 48)
            return TOO_SMALL;

        uint8_t amount_buf[8];
        INT64_TO_BUF(amount_buf, amt->iou.amount);
        memcpy(write_ptr, amount_buf, 8);
        memcpy(write_ptr + 8, amt->iou.currency, 3);
        memcpy(write_ptr + 28, amt->iou.issuer, 20);
        return 48;
    }
}
int64_t slot_float(uint32_t slot)
{
    struct Amount *amt = (struct Amount *)slot;
    if (amt->is_xrp)
        return amt->xrp.amount;
    else
        return amt->iou.amount;
}
int64_t slot_subfield(uint32_t parent_slot, uint32_t field_id, uint32_t new_slot)
{
    if (field_id == sfAmount)
    {
        struct Transaction *tx = (struct Transaction *)parent_slot;
        return &tx->amount;
    }

    return -1;
}
int64_t slot_type(uint32_t slot, uint32_t flags)
{
    if (flags == 1) // We only intrested in detecting whether amount is xrp or not.
    {
        struct Amount *amt = (struct Amount *)slot;
        return amt->is_xrp;
    }
    return 0;
}
int64_t slot_set(uint32_t read_ptr, uint32_t read_len, int32_t slot)
{
    return 0;
}
int64_t sto_subarray(uint32_t read_ptr, uint32_t read_len, uint32_t array_id)
{
    return 0;
}
int64_t sto_subfield(uint32_t read_ptr, uint32_t read_len, uint32_t field_id)
{
    return 0;
}

int64_t trace(uint32_t mread_ptr, uint32_t mread_len, uint32_t dread_ptr, uint32_t dread_len, uint32_t as_hex)
{
    if (as_hex == 1)
        printf("%*.*s %*.*X\n", 0, mread_len, mread_ptr, 0, dread_len, dread_ptr);
    else
        printf("%*.*s %*.*s\n", 0, mread_len, mread_ptr, 0, dread_len, dread_ptr);
    return 0;
}

int64_t trace_num(uint32_t read_ptr, uint32_t read_len, int64_t number)
{
    printf("%*.*s %lld\n", 0, read_len, read_ptr, number);
    return 0;
}

int64_t trace_float(uint32_t mread_ptr, uint32_t mread_len, int64_t float1)
{
    printf("%*.*s %lld\n", 0, mread_len, mread_ptr, float1);
    return 0;
}

// JS

int64_t emit(uint32_t write_ptr, uint32_t write_len, uint32_t read_ptr, uint32_t read_len)
{
    return 0;
}

// Reading trustline data.
int64_t util_keylet(uint32_t write_ptr, uint32_t write_len, uint32_t keylet_type, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f)
{
    return 0;
}
// 1. send the request to console.
// 2. listen to stdin for response.

int64_t state_set(uint32_t read_ptr, uint32_t read_len, uint32_t kread_ptr, uint32_t kread_len)
{
    return 0;
}
// 1. Send the request to JS.
// 2. Keep data in memory until the C program finishes.
// 3. Commit to sqlite based on the return code of C program.

int64_t state(uint32_t write_ptr, uint32_t write_len, uint32_t kread_ptr, uint32_t kread_len)
{
    return 0;
}
// 1. Send the request to JS.
// 2. First look in in memory location.
// 3. If not found, look in sqlite.
// 4. If not found, send NOT_FOUND.

int64_t rollback(uint32_t read_ptr, uint32_t read_len, int64_t error_code)
{
    trace(SBUF("rollback: "), read_ptr, read_len, 0);
    exit(-1);
    return 0;
}
