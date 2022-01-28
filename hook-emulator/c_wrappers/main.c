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

static uint8_t const emit_details_len = 105;
static uint8_t const hash_len = 32;
static uint8_t const state_key_len = 32;
static uint8_t const max_state_val_len = 128;

#define DOESNT_EXIST -5

// ------------------ Message formats ------------------
// Message protocol - <data len(4)><data>
// -----------------------------------------------------

// --------------- Message data formats ----------------

// ''''' JS --> C_WRAPPER (Write to STDIN from JS) '''''
// Transaction origin - <hookid(20)><account(20)><1 for xrp and 0 for iou(1)><[XRP: amount in buf(8)XFL][IOU: <issuer(20)><currency(3)><amount in buf(8)XFL>]><destination(20)><memo count(1)><[<TypeLen(1)><MemoType(20)><FormatLen(1)><MemoFormat(20)><DataLen(1)><MemoData(128)>]><ledger_hash(32)><ledger_index(8)>
// Keylet response - <issuer(20)><currency(3)><balance(8)XFL><limit(8)XFL>
// State get response - <value(128)>
// '''''''''''''''''''''''''''''''''''''''''''''''''''''

// ' C_WRAPPER --> JS (Write to STDOUT from C_WRAPPER) '
// Trace - <TYPE:TRACE(1)><trace message>
// Transaction emit - <TYPE:EMIT(1)><prepared transaction buf>
// Keylet request - <TYPE:KEYLET(1)><issuer(20)><currency(3)>
// State set request - <TYPE:STATE_GET(1)><key(32)><value(128)>
// State get request - <TYPE:STATE_SET(1)><key(32)>
// '''''''''''''''''''''''''''''''''''''''''''''''''''''
// -----------------------------------------------------

void write_stdout(const uint8_t *buf, const int len)
{
    const int outlen = 4 + len;
    char out[outlen];
    out[0] = len >> 24;
    out[1] = len >> 16;
    out[2] = len >> 8;
    out[3] = len;
    for (int i = 0; i < len; i++)
        out[i + 4] = buf[i];

    write(STDOUT_FILENO, out, outlen);
    fflush(stdout);
}

void trace_out(const char *trace)
{
    const int len = 1 + strlen(trace);
    char buf[len];
    buf[0] = TRACE;
    sprintf(&buf[1], "%s", trace);
    write_stdout(buf, len);
}

int64_t trace(uint32_t mread_ptr, uint32_t mread_len, uint32_t dread_ptr, uint32_t dread_len, uint32_t as_hex)
{
    if (as_hex == 1)
    {
        char out[mread_len + (dread_len * 2) + 1];
        sprintf(out, "%*.*s %*.*X", 0, mread_len, mread_ptr, 0, dread_len, dread_ptr);
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
    // Data length is added from the protocol in first 4 bytes, so we skip those.
    uint8_t read_buf[4 + hash_len];
    read(STDIN_FILENO, read_buf, sizeof(read_buf));
    uint32_t data_len = read_buf[3] | (read_buf[2] << 8) | (read_buf[1] << 16) | (read_buf[0] << 24);

    if (data_len == hash_len)
    {
        write_len = data_len;
        memcpy(write_ptr, read_buf + 4, write_len);
        return 1;
    }
    else
    {
        write_ptr = 0;
        write_len = 0;
        return -1;
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
    // Data length is added from the protocol in first 4 bytes, so we skip those.
    uint8_t read_buf[5];
    read(STDIN_FILENO, read_buf, sizeof(read_buf));
    uint32_t data_len = read_buf[3] | (read_buf[2] << 8) | (read_buf[1] << 16) | (read_buf[0] << 24);

    if (data_len == 1)
        return ((int8_t)read_buf[4] == 0) ? 0 : -1;
    else
        return -1;
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
    // Data length is added from the protocol in first 4 bytes, so we skip those.
    uint8_t read_buf[4 + max_state_val_len];
    read(STDIN_FILENO, read_buf, sizeof(read_buf));
    uint32_t data_len = read_buf[3] | (read_buf[2] << 8) | (read_buf[1] << 16) | (read_buf[0] << 24);

    if (data_len > 0)
    {
        write_len = data_len;
        memcpy(write_ptr, read_buf + 4, write_len);
        return write_len;
    }
    else
    {
        write_ptr = 0;
        write_len = 0;
        return DOESNT_EXIST;
    }
}

int keylet(uint8_t *lines, const uint8_t *issuer, const uint8_t *currency)
{
    const int len = 24;
    uint8_t buf[len];
    buf[0] = KEYLET;
    memcpy(&buf[1], issuer, 20);
    memcpy(&buf[21], currency, 3);
    write_stdout(buf, len);

    uint8_t read_buf[53];
    read(STDIN_FILENO, read_buf, sizeof(read_buf));
    uint32_t lines_len = read_buf[3] | (read_buf[2] << 8) | (read_buf[1] << 16) | (read_buf[0] << 24);

    memcpy(lines, read_buf + 4, lines_len);

    return lines_len;
}

int main()
{
    uint8_t buf[200000];
    char *format = "";
    int len = 0;

    trace(SBUF("Enter a string: "), 0, 0, 0);

    read(STDIN_FILENO, buf, sizeof(buf));
    const uint32_t read_len = buf[3] | (buf[2] << 8) | (buf[1] << 16) | (buf[0] << 24);
    const uint32_t tx_len = read_len - 20;

    uint8_t hook_accid[20];
    uint8_t tx[tx_len];
    memcpy(hook_accid, &buf[4], 20);
    memcpy(tx, &buf[24], tx_len);

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
    const int lines_len = keylet(lines, account_field, "ABC");

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