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
    RES_NOT_FOUND = -2
};

static uint8_t const emit_details_len = 105;
static uint8_t const hash_len = 32;
static uint8_t const state_key_len = 32;
static uint8_t const max_state_val_len = 128;

#define MAX_READ_LEN 1024

#define DOESNT_EXIST -5 // This can be found in the hook_macro when it's included.

// ------------------ Message formats ------------------
// Message protocol - <data len(4)><data>
// -----------------------------------------------------

// --------------- Message data formats ----------------

// ''''' JS --> C_WRAPPER (Write to STDIN from JS) '''''
// Transaction origin - <hookid(20)><account(20)><1 for xrp and 0 for iou(1)><[XRP: amount in buf(8)XFL][IOU: <issuer(20)><currency(3)><amount in buf(8)XFL>]><destination(20)><memo count(1)><[<TypeLen(1)><MemoType(20)><FormatLen(1)><MemoFormat(20)><DataLen(1)><MemoData(128)>]><ledger_hash(32)><ledger_index(8)>
// Emit response - <return code(1)><tx hash(32)>
// Keylet response - <return code(1)><issuer(20)><currency(3)><balance(8)XFL><limit(8)XFL>
// State get response - <return code(1)><value(128)>
// State set response - <return code(1)>
// '''''''''''''''''''''''''''''''''''''''''''''''''''''

// ' C_WRAPPER --> JS (Write to STDOUT from C_WRAPPER) '
// Trace - <TYPE:TRACE(1)><trace message>
// Transaction emit - <TYPE:EMIT(1)><prepared transaction buf>
// Keylet request - <TYPE:KEYLET(1)><issuer(20)><currency(3)>
// State set request - <TYPE:STATE_GET(1)><key(32)><value(128)>
// State get request - <TYPE:STATE_SET(1)><key(32)>
// '''''''''''''''''''''''''''''''''''''''''''''''''''''
// -----------------------------------------------------

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

    return (write(STDOUT_FILENO, out, outlen) == -1 || fflush(stdout) == -1) ? -1 : 0;
}

int read_stdin(uint8_t **read_buf)
{
    // Read the response from STDIN.
    // Data length is added from the protocol in first 4 bytes, so we skip those.
    uint8_t buf[MAX_READ_LEN];

    if (read(STDIN_FILENO, buf, sizeof(buf)) == -1)
        return -1;

    // Read the data length.
    uint32_t len = buf[3] | (buf[2] << 8) | (buf[1] << 16) | (buf[0] << 24);

    // Populate the data to buffer.
    *read_buf = (uint8_t *)malloc(len);
    memcpy(*read_buf, &buf[4], len);

    return len;
}

int trace_out(const char *trace)
{
    const int len = 1 + strlen(trace);
    char buf[len];
    buf[0] = TRACE;
    sprintf(&buf[1], "%s", trace);
    return write_stdout(buf, len);
}

int fetch_result(const uint8_t *data_buf, const uint64_t data_len, uint8_t **result, uint64_t *result_len)
{
    const int ret_val = (int8_t)*data_buf;
    if (ret_val < 0)
        return ret_val;

    *result_len = data_len - 1;
    if (*result_len > 0)
    {
        *result = (uint8_t *)malloc(*result_len);
        memcpy(*result, data_buf + 1, *result_len);
    }
    else
        *result = 0;
    return ret_val;
}

int64_t trace(uint32_t mread_ptr, uint32_t mread_len, uint32_t dread_ptr, uint32_t dread_len, uint32_t as_hex)
{
    if (as_hex == 1)
    {
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
    char out[500];
    sprintf(out, "%*.*s %lld", 0, mread_len, mread_ptr, float1);
    trace_out(out);
    return 0;
}

int64_t emit(uint32_t write_ptr, uint32_t write_len, uint32_t read_ptr, uint32_t read_len)
{
    const int buflen = 1 + read_len - emit_details_len;
    uint8_t buf[buflen];
    buf[0] = EMIT;
    memcpy(&buf[1], read_ptr, read_len - emit_details_len);
    write_stdout(buf, buflen);

    // Read the response from STDIN.
    uint8_t *data_buf, *res;
    uint64_t res_len;
    uint64_t data_len = read_stdin(&data_buf);
    int ret = fetch_result(data_buf, data_len, &res, &res_len);

    if (ret < 0)
    {
        write_ptr = 0;
        return -1;
    }
    else
    {
        if (res_len > 0)
            memcpy(write_ptr, res, res_len);
        else
            write_ptr = 0;
        return 1;
    }
}

int keylet(uint32_t write_ptr, uint32_t write_len, const uint8_t *issuer, const uint8_t *currency)
{
    const int len = 24;
    uint8_t buf[len];
    buf[0] = KEYLET;
    memcpy(&buf[1], issuer, 20);
    memcpy(&buf[21], currency, 3);
    write_stdout(buf, len);

    // Read the response from STDIN.
    uint8_t *data_buf, *res;
    uint64_t res_len;
    uint64_t data_len = read_stdin(&data_buf);
    int ret = fetch_result(data_buf, data_len, &res, &res_len);

    if (ret < 0)
    {
        write_ptr = 0;
        return -1;
    }
    else
    {
        if (res_len > 0)
            memcpy(write_ptr, res, res_len);
        else
            write_ptr = 0;
        return 1;
    }
}

int64_t state_set(uint32_t read_ptr, uint32_t read_len, uint32_t kread_ptr, uint32_t kread_len)
{
    if (kread_len != state_key_len)
        return -1;

    const int len = 1 + kread_len + read_len;
    uint8_t buf[len];
    buf[0] = STATE_SET;
    memcpy(&buf[1], kread_ptr, kread_len);
    memcpy(&buf[1 + kread_len], read_ptr, read_len);
    write_stdout(buf, len);

    // Read the response from STDIN.
    // Read the response from STDIN.
    uint8_t *data_buf, *res;
    uint64_t res_len;
    uint64_t data_len = read_stdin(&data_buf);
    int ret = fetch_result(data_buf, data_len, &res, &res_len);

    return (ret < 0) ? -1 : 1;
}

int64_t state(uint32_t write_ptr, uint32_t write_len, uint32_t kread_ptr, uint32_t kread_len)
{
    if (kread_len != state_key_len)
        return -1;

    const int len = 1 + kread_len;
    uint8_t buf[len];
    buf[0] = STATE_GET;
    memcpy(&buf[1], kread_ptr, kread_len);
    write_stdout(buf, len);

    // Read the response from STDIN.
    uint8_t *data_buf, *res;
    uint64_t res_len;
    uint64_t data_len = read_stdin(&data_buf);
    int ret = fetch_result(data_buf, data_len, &res, &res_len);

    if (ret < 0)
    {
        write_ptr = 0;
        return (ret == RES_NOT_FOUND) ? DOESNT_EXIST : -1;
    }
    else
    {
        if (res_len > 0)
            memcpy(write_ptr, res, res_len);
        else
            write_ptr = 0;
        return 1;
    }
}

int main()
{
    char *format = "";
    int len = 0;

    trace(SBUF("Enter a string: "), 0, 0, 0);

    uint8_t *data_buf;
    int data_len = read_stdin(&data_buf);

    const uint32_t tx_len = data_len - 20;

    uint8_t hook_accid[20];
    uint8_t tx[tx_len];
    memcpy(hook_accid, data_buf, 20);
    memcpy(tx, data_buf + 20, tx_len);

    const int account_field_offset = 0;
    uint8_t account_field[20];
    memcpy(account_field, tx, 20);

    int to_acc_offset = 52;
    uint8_t to_acc[20];
    if (tx[21] == 0)
        memcpy(to_acc, tx, to_acc_offset);
    else
    {
        to_acc_offset = 29;
        memcpy(to_acc, tx, to_acc_offset);
    }

    trace(SBUF("Hook"), SBUF(hook_accid), 1);
    trace(SBUF("Transaction"), SBUF(tx), 1);

    uint8_t lines[39];
    const int lines_len = keylet(SBUF(lines), account_field, "ABC");

    trace(SBUF("Trustlines"), SBUF(lines), 1);

    // etxn_reserve(1);

    // //////////////// Payment ////////////////

    // int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_SIZE);

    // // Finally create the outgoing txn.
    // uint8_t txn_out[PREPARE_PAYMENT_SIMPLE_SIZE];
    // PREPARE_PAYMENT_SIMPLE(txn_out, 1, fee, account_field, 0, 0);

    // uint8_t emithash[32];
    // if (emit(SBUF(emithash), SBUF(txn_out)) < 0)
    //     rollback(SBUF("Evernode: Emitting transaction failed."), 1);

    // //////////////// Trustline ////////////////

    // // Calculate fee for trustline transaction.
    // int64_t fee = etxn_fee_base(PREPARE_SIMPLE_TRUSTLINE_SIZE);

    // uint8_t amt_out[48] = {
    //     213, 67, 141, 126, 164, 198, 128, 0, 0, 0, 0,
    //     0, 0, 0, 0, 0, 0, 0, 0, 0, 88, 89, 90, 0, 0, 0, 0, 0, 176, 206, 249, 3, 178,
    //     132, 222, 155, 235, 20, 236, 133, 1, 41, 70, 140,
    //     185, 225, 186, 216};

    // // Preparing trustline transaction.
    // uint8_t trust_out[PREPARE_SIMPLE_TRUSTLINE_SIZE];
    // PREPARE_SIMPLE_TRUSTLINE(trust_out, amt_out, fee);

    // uint8_t emithash[32];
    // if (emit(SBUF(emithash), SBUF(trust_out)) < 0)
    //     rollback(SBUF("Evernode: Emitting txn failed"), 1);

    // //////////////// Check ////////////////

    // int64_t fee = etxn_fee_base(PREPARE_SIMPLE_CHECK_SIZE);

    // uint8_t chk_amt_out[48] = {
    //     213, 67, 141, 126, 164, 198, 128, 0, 0, 0, 0,
    //     0, 0, 0, 0, 0, 0, 0, 0, 0, 88, 89, 90, 0, 0, 0, 0, 0, 176, 206, 249, 3, 178,
    //     132, 222, 155, 235, 20, 236, 133, 1, 41, 70, 140,
    //     185, 225, 186, 216};
    // // Finally create the outgoing txn.
    // uint8_t chk_out[PREPARE_SIMPLE_CHECK_SIZE];
    // PREPARE_SIMPLE_CHECK(chk_out, chk_amt_out, fee, account_field);

    // uint8_t emithash[32];
    // if (emit(SBUF(emithash), SBUF(chk_out)) < 0)
    //     rollback(SBUF("Evernode: Emitting transaction failed."), 1);

    // //////////////// Trust Transaction ////////////////

    // int64_t fee = etxn_fee_base(PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE);

    // uint8_t trst_amt_out[48] = {
    //     213, 67, 141, 126, 164, 198, 128, 0, 0, 0, 0,
    //     0, 0, 0, 0, 0, 0, 0, 0, 0, 88, 89, 90, 0, 0, 0, 0, 0, 176, 206, 249, 3, 178,
    //     132, 222, 155, 235, 20, 236, 133, 1, 41, 70, 140,
    //     185, 225, 186, 216};
    // // Finally create the outgoing txn.
    // uint8_t trst_out[PREPARE_PAYMENT_SIMPLE_TRUSTLINE_SIZE];
    // PREPARE_PAYMENT_SIMPLE_TRUSTLINE(trst_out, trst_amt_out, fee, account_field, 0, 0);

    // uint8_t emithash[32];
    // if (emit(SBUF(emithash), SBUF(trst_out)) < 0)
    //     rollback(SBUF("Evernode: Emitting transaction failed."), 1);

    // //////////////// Trust Transaction ////////////////

    const uint8_t STATEKEY[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

    uint8_t res_buf[8];
    int64_t res = 0;
    if (state(SBUF(res_buf), SBUF(STATEKEY)) == DOESNT_EXIST)
        trace(SBUF("No state."), 0, 0, 0);
    else
    {
        res = INT64_FROM_BUF(res_buf);
        trace_num(SBUF("Before value"), res);
    }

    uint8_t seq_buf[8];
    memcpy(seq_buf, &tx[tx_len - 8], 8);
    if (state_set(SBUF(seq_buf), SBUF(STATEKEY)) < 0)
    {
        trace(SBUF("Could not create the state."), 0, 0, 0);
        exit(-1);
    }

    res = 0;
    if (state(SBUF(res_buf), SBUF(STATEKEY)) == DOESNT_EXIST)
        trace(SBUF("No state."), 0, 0, 0);
    else
    {
        res = INT64_FROM_BUF(res_buf);
        trace_num(SBUF("After value"), res);
    }

    exit(0);
}