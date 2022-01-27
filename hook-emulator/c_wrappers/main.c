#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>

enum MESSAGE_TYPES
{
    TRACE,
    EMIT,
    KEYLET,
    STATE_GET,
    STATE_SET
};

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

const uint8_t STATEKEY[32] = {'E', 'V', 'R', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

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

void trace(const char *trace)
{
    const int len = 1 + strlen(trace);
    char buf[len];
    buf[0] = TRACE;
    sprintf(&buf[1], "%s", trace);
    write_stdout(buf, len);
}

void emit(const uint8_t *tx, const int len)
{
    const int buflen = 1 + len;
    uint8_t buf[buflen];
    buf[0] = EMIT;
    memcpy(&buf[1], tx, buflen);
    write_stdout(buf, buflen);
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

void state_set(const uint8_t *key, const uint8_t *value, const int value_len)
{
    const int len = 33 + value_len;
    uint8_t buf[len];
    buf[0] = STATE_SET;
    memcpy(&buf[1], key, 32);
    memcpy(&buf[33], value, value_len);
    write_stdout(buf, len);
}

int state_get(uint8_t *value, const uint8_t *key)
{
    const int len = 33;
    uint8_t buf[len];
    buf[0] = STATE_GET;
    memcpy(&buf[1], key, 32);
    write_stdout(buf, len);

    uint8_t read_buf[53];
    read(STDIN_FILENO, read_buf, sizeof(read_buf));
    uint32_t value_len = read_buf[3] | (read_buf[2] << 8) | (read_buf[1] << 16) | (read_buf[0] << 24);

    memcpy(value, read_buf + 4, value_len);

    return value_len;
}

void bin_to_hex(char *hex, const uint8_t *bin, const int len)
{
    strcpy(hex, "");
    for (int i = 0; i < len; i++)
    {
        char n[20];
        sprintf(n, "%02X", bin[i]);
        strcat(hex, n);
    }
    strcat(hex, "\0");
}

int main()
{
    uint8_t buf[200000];
    char *format = "";
    int len = 0;

    trace("Enter a string: ");

    read(STDIN_FILENO, buf, sizeof(buf));
    const uint32_t read_len = buf[3] | (buf[2] << 8) | (buf[1] << 16) | (buf[0] << 24);
    const uint32_t tx_len = read_len - 20;

    uint8_t hook_accid[20];
    uint8_t tx[tx_len];
    memcpy(hook_accid, &buf[4], 20);
    memcpy(tx, &buf[24], tx_len);

    const int from_acc_offset = 0;
    uint8_t from_acc[20];
    memcpy(from_acc, tx, 20);

    int to_acc_offset = 52;
    uint8_t to_acc[20];
    if (tx[21] == 0)
        memcpy(to_acc, tx, to_acc_offset);
    else
    {
        to_acc_offset = 29;
        memcpy(to_acc, tx, to_acc_offset);
    }

    uint8_t token[3];
    token[0] = 'A';
    token[1] = 'B';
    token[2] = 'C';

    char bytes[2048];
    bin_to_hex(bytes, hook_accid, 20);
    format = "Hook: %s";
    len = 13 + strlen(bytes);
    char out[len];
    sprintf(out, format, bytes);
    trace(out);

    bin_to_hex(bytes, tx, tx_len);
    format = "Transaction: %s";
    len = 13 + strlen(bytes);
    char out2[len];
    sprintf(out2, format, bytes);
    trace(out2);

    uint8_t lines[39];
    const int lines_len = keylet(lines, from_acc, token);

    bin_to_hex(bytes, lines, lines_len);
    format = "Trustlines: %s";
    len = 13 + strlen(bytes);
    char out3[len];
    sprintf(out3, format, bytes);
    trace(out3);

    uint8_t state[128];
    int state_len = state_get(state, STATEKEY);
    if (state_len > 0)
    {
        uint64_t state_val = state[7] | (state[6] << 8) | (state[5] << 16) | (state[4] << 24) | (state[3] << 32) | (state[2] << 40) | (state[1] << 48) | (state[0] << 56);
        format = "Before state val: %d";
        len = 100;
        char out4[len];
        sprintf(out4, format, state_val);
        trace(out4);
    }
    else
        trace("Before state val is empty");

    state_set(STATEKEY, &tx[tx_len - 8], 8);

    state_len = state_get(state, STATEKEY);
    if (state_len > 0)
    {
        uint64_t state_val = state[7] | (state[6] << 8) | (state[5] << 16) | (state[4] << 24) | (state[3] << 32) | (state[2] << 40) | (state[1] << 48) | (state[0] << 56);
        format = "After state val: %d";
        len = 100;
        char out4[len];
        sprintf(out4, format, state_val);
        trace(out4);
    }
    else
        trace("After state val is empty");

    uint64_t ledger_idx = tx[tx_len - 8 + 7] | (tx[tx_len - 8 + 6] << 8) | (tx[tx_len - 8 + 5] << 16) | (tx[tx_len - 8 + 4] << 24) | (tx[tx_len - 8 + 3] << 32) | (tx[tx_len - 8 + 2] << 40) | (tx[tx_len - 8 + 1] << 48) | (tx[tx_len - 8 + 0] << 56);
    if (ledger_idx % 2 == 0)
        exit(-1);

    exit(0);
}