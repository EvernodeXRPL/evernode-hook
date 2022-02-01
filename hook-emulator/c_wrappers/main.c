#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>

#include "../../lib/hookmacro.h";
// #include "../../src/constants.h"
// #include "../../src/evernode.h"
// #include "../../src/statekeys.h"

enum MESSAGE_TYPES
{
    TRACE,
    EMIT,
    KEYLET,
    STATE_GET,
    STATE_SET
};

enum RETURN_CODES
{
    RES_SUCCESS = 0,
    RES_INTERNAL_ERROR = -1,
    RES_NOT_FOUND = -2,
    RET_OVERFLOW = -3,
    RET_UNDERFLOW = -4
};

#define EMIT_DETAILS_LEN 105
#define TRANSACTION_HASH_LEN 32
#define STATE_KEY_LEN 32
#define MAX_STATE_VAL_LEN 128
#define MAX_READ_LEN 1024
#define TRUSTLINE_LEN 16

struct etxn_info
{
    uint32_t etxn_reserves;
    int etxn_reserved;
}

etxn_inf = {0, 0};

// --------------- Message data formats ----------------

// ''''' JS --> C_WRAPPER (Write to STDIN from JS) '''''
// Transaction origin - <hookid(20)><account(20)><1 for xrp and 0 for iou(1)><[XRP: amount in buf(8)XFL][IOU: <issuer(20)><currency(3)><amount in buf(8)XFL>]><destination(20)><memo count(1)><[<TypeLen(1)><MemoType(20)><FormatLen(1)><MemoFormat(20)><DataLen(1)><MemoData(128)>]><ledger_hash(32)><ledger_index(8)>
// Message response format - <RETURN CODE(1)><DATA>
// Emit response - <return code(1)><tx hash(32)>
// Trustline response - <return code(1)><balance(8)XFL><limit(8)XFL>
// State get response - <return code(1)><value(128)>
// State set response - <return code(1)>
// '''''''''''''''''''''''''''''''''''''''''''''''''''''

// ' C_WRAPPER --> JS (Write to STDOUT from C_WRAPPER) '
// Message protocol - <data len(4)><data>
// Message request format - <TYPE(1)><DATA>
// Trace request - <type:TRACE(1)><trace message>
// Transaction emit request - <type:EMIT(1)><prepared transaction buf>
// Trustline request - <type:TRUSTLINE(1)><address(20)><issuer(20)><currency(3)>
// State set request - <type:STATE_GET(1)><key(32)><value(128)>
// State get request - <type:STATE_SET(1)><key(32)>
// '''''''''''''''''''''''''''''''''''''''''''''''''''''
// -----------------------------------------------------

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

/**
 * Get trustlines of the running account.
 * @param address Source address of the trustlines.
 * @param issuer Issuer of the trustline currency.
 * @param currency Issued currency.
 * @param balance_float Float balance to be populated.
 * @param limit_float Limit float to be populated.
*/
int get_trustlines(const uint8_t *address, const uint8_t *issuer, const uint8_t *currency, int64_t *balance_float, int64_t *limit_float)
{
    // Send the emit request.
    const int len = 44;
    uint8_t buf[len];
    // Populate the type header.
    buf[0] = KEYLET;
    // Populate the address, issuer and currency.
    memcpy(&buf[1], address, 20);
    memcpy(&buf[21], issuer, 20);
    memcpy(&buf[41], currency, 3);
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

int64_t trace(uint32_t mread_ptr, uint32_t mread_len, uint32_t dread_ptr, uint32_t dread_len, uint32_t as_hex)
{
    if (as_hex == 1)
    {
        // If hex (byte => 2 ascci characters) allocate a buffer and populate the hex characters in the loop.
        char out[mread_len + (dread_len * 2) + 1];
        sprintf(out, "%*.*s ", 0, mread_len, mread_ptr);
        for (int i = 0; i < dread_len; i++)
            sprintf(&out[mread_len + (i * 2)], "%02X", *(uint8_t *)(dread_ptr + i));
        trace_out(out);
    }
    else
    {
        char out[mread_len + dread_len + 1];
        sprintf(out, "%*.*s %*.*s", 0, mread_len, mread_ptr, 0, dread_len, dread_ptr);
        trace_out(out);
    }
    return 0;
}

int64_t trace_num(uint32_t read_ptr, uint32_t read_len, int64_t number)
{
    char out[500];
    sprintf(out, "%*.*s %lld", 0, read_len, read_ptr, number);
    trace_out(out);
    return 0;
}

int64_t trace_float(uint32_t mread_ptr, uint32_t mread_len, int64_t float1)
{
    const uint64_t mantissa = get_mantissa(float1);
    char out[500];
    sprintf(out, (mantissa != 0 && is_negative(float1)) ? "%*.*s Float -%lld*10^(%ld)" : "%*.*s Float %lld*10^(%ld)",
            0, mread_len, mread_ptr, mantissa, get_exponent(float1));
    trace_out(out);
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
    memcpy(&buf[1], read_ptr, buflen - 1);
    write_stdout(buf, buflen);

    // Read the response from STDIN.
    uint8_t data_buf[TRANSACTION_HASH_LEN + 1];
    const int data_len = read_stdin(data_buf, sizeof(data_buf));
    const int ret = (int8_t)*data_buf;
    const uint8_t *res = &data_buf[1];

    // If result code is 0 return EMISSION_FAILURE.
    if (ret < 0 || (data_len - 1) != TRANSACTION_HASH_LEN)
    {
        write_ptr = 0;
        return EMISSION_FAILURE;
    }

    // Populate the received transaction hash to the write pointer.
    memcpy(write_ptr, res, TRANSACTION_HASH_LEN);
    return 0;
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
    memcpy(&buf[1], kread_ptr, kread_len);
    // If kread_len is less than STATE_KEY_LEN, populate o's to the rest.
    if (kread_len < STATE_KEY_LEN)
        memset(&buf[1 + kread_len], 0, STATE_KEY_LEN - kread_len);
    // Populate the data to rest.
    memcpy(&buf[1 + STATE_KEY_LEN], read_ptr, read_len);
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
    memcpy(&buf[1], kread_ptr, kread_len);
    write_stdout(buf, len);

    // Read the response from STDIN.
    uint8_t data_buf[MAX_STATE_VAL_LEN];
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
        memcpy(write_ptr, res, (data_len - 1)); // Populate only if res_len > 0;
    else
        write_ptr = 0;

    return 0;
}

/**
 * Temporary main method for testing.
*/
int main()
{
    // Trace test.
    trace(SBUF("Enter a string: "), 0, 0, 0);

    // Read the transaction from STDIN.
    uint8_t data_buf[MAX_READ_LEN];
    const int data_len = read_stdin(data_buf, sizeof(data_buf));

    // Test transaction content.
    const uint32_t tx_len = data_len - 20;
    uint8_t hook_accid[20];
    uint8_t tx[tx_len];

    // Get hook account.
    memcpy(hook_accid, data_buf, 20);
    // Get transaction.
    memcpy(tx, data_buf + 20, tx_len);

    const int account_field_offset = 0;
    uint8_t account_field[20];
    memcpy(account_field, tx, 20);

    trace(SBUF("Hook"), SBUF(hook_accid), 1);
    trace(SBUF("Account"), SBUF(account_field), 1);

    // Test trustlines 1.
    int64_t balance, limit;
    int trustline_res = get_trustlines(hook_accid, account_field, "XYZ", &balance, &limit);
    if (trustline_res == DOESNT_EXIST)
        trace(SBUF("No in trustlines."), 0, 0, 0);
    else if (trustline_res < 0)
        trace_num(SBUF("In trustline error"), trustline_res);
    else
    {
        trace_float(SBUF("In trustlines balance"), balance);
        trace_float(SBUF("In trustlines limit"), limit);
    }

    // Test trustlines 2.
    trustline_res = get_trustlines(account_field, hook_accid, "XYZ", &balance, &limit);
    if (trustline_res == DOESNT_EXIST)
        trace(SBUF("No out trustlines."), 0, 0, 0);
    else if (trustline_res < 0)
        trace_num(SBUF("Out trustline error"), trustline_res);
    else
    {
        trace_float(SBUF("Out trustlines balance"), balance);
        trace_float(SBUF("Out trustlines limit"), limit);
    }

    // Test state set and get.

    const uint8_t STATEKEY[25] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

    // Get state example - check the state value before state set.
    uint8_t res_buf[8];
    int64_t res = 0;
    int state_res = state(SBUF(res_buf), SBUF(STATEKEY));
    if (state_res < 0)
    {
        if (state_res == DOESNT_EXIST)
            trace(SBUF("No state."), 0, 0, 0);
        else
            trace_num(SBUF("State error"), state_res);
    }
    else
    {
        res = INT64_FROM_BUF(res_buf);
        trace_num(SBUF("Before value"), res);
    }

    // Set state example - set the ledger seq in the transaction as state.
    uint8_t seq_buf[8];
    memcpy(seq_buf, &tx[tx_len - 8], 8);
    if (state_set(SBUF(seq_buf), SBUF(STATEKEY)) < 0)
    {
        trace(SBUF("Could not create the state."), 0, 0, 0);
        exit(-1);
    }

    // Get state example - check the state value after setting.
    res = 0;
    state_res = state(SBUF(res_buf), SBUF(STATEKEY));
    if (state_res < 0)
    {
        if (state_res == DOESNT_EXIST)
            trace(SBUF("No state."), 0, 0, 0);
        else
            trace_num(SBUF("State error"), state_res);
    }
    else
    {
        res = INT64_FROM_BUF(res_buf);
        trace_num(SBUF("After value"), res);
    }

    ////////////////////////////////////////////

    // etxn_reserve(4);

    // // //////////////// Payment ////////////////

    // int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_SIZE);

    // // Finally create the outgoing txn.
    // uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_SIZE];
    // PREPARE_PAYMENT_SIMPLE(txn_out, 1, fee, account_field, 0, 0);

    // uint8_t emithash1[32];
    // if (emit(SBUF(emithash1), SBUF(txn_out)) < 0)
    //     rollback(SBUF("Emitting transaction failed."), 1);
    // trace(SBUF("payment emit hash"), SBUF(emithash1), 1);

    // // //////////////// Trustline ////////////////

    // // // Calculate fee for trustline transaction.
    // fee = etxn_fee_base(PREPARE_SIMPLE_TRUSTLINE_SIZE);

    // int64_t trust_limit = float_set(3, 10);
    // uint8_t amt_out[AMOUNT_BUF_SIZE];
    // SET_AMOUNT_OUT(amt_out, "XYZ", account_field, trust_limit);

    // // Preparing trustline transaction.
    // uint8_t trust_out[PREPARE_SIMPLE_TRUSTLINE_SIZE];
    // PREPARE_SIMPLE_TRUSTLINE(trust_out, amt_out, fee);

    // uint8_t emithash2[32];
    // if (emit(SBUF(emithash2), SBUF(trust_out)) < 0)
    //     rollback(SBUF("Emitting txn failed"), 1);
    // trace(SBUF("trustline emit hash"), SBUF(emithash2), 1);

    // // //////////////// Check ////////////////

    // fee = etxn_fee_base(PREPARE_SIMPLE_CHECK_SIZE);

    // uint8_t chk_amt_out[48];
    // // Finally create the outgoing txn.
    // int64_t amount = float_set(1, 1);
    // SET_AMOUNT_OUT(chk_amt_out, "XYZ", account_field, amount);

    // uint8_t chk_out[PREPARE_SIMPLE_CHECK_SIZE];
    // PREPARE_SIMPLE_CHECK(chk_out, chk_amt_out, fee, account_field);

    // uint8_t emithash3[32];
    // if (emit(SBUF(emithash3), SBUF(chk_out)) < 0)
    //     rollback(SBUF("Emitting transaction failed."), 1);
    // trace(SBUF("check emit hash"), SBUF(emithash3), 1);

    // // //////////////// Trust Payment ////////////////

    // fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

    // amount = float_set(1, 10);
    // uint8_t trst_amt_out[AMOUNT_BUF_SIZE];
    // SET_AMOUNT_OUT(trst_amt_out, "XYZ", account_field, amount);

    // // Finally create the outgoing txn.
    // uint8_t trst_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
    // PREPARE_PAYMENT_SIMPLE_TRUSTLINE(trst_out, trst_amt_out, fee, account_field, 0, 0);

    // uint8_t emithash4[32];
    // if (emit(SBUF(emithash4), SBUF(trst_out)) < 0)
    //     rollback(SBUF("Emitting transaction failed."), 1);
    // trace(SBUF("trust payment emit hash"), SBUF(emithash4), 1);

    // // /////////////////////////////////////////////////////

    exit(0);
}