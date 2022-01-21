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
    STATEGET,
    STATESET
};

// Transaction (origin|emit) - <account(20)><1 for xrp and 0 for iou(1)><[XRP: amount in buf(8)XFL][IOU: <issuer(20)><currency(3)><amount in buf(8)XFL>]><destination(20)><memo count(1)><[<TypeLen(1)><MemoType(20)><FormatLen(1)><MemoFormat(20)><DataLen(1)><MemoData(128)>]><ledger_hash(32)><ledger_index(8)>
// Keylet request - <issuer(20)><currency(3)>
// Keylet response - <issuer(20)><currency(3)><balance(8)XFL><limit(8)XFL>

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
    int buflen = 1 + len;
    uint8_t buf[buflen];
    buf[0] = EMIT;
    for (int i = 1; i < buflen; i++)
        buf[i] = tx[i - 1];

    write_stdout(buf, buflen);
}

int keylet(uint8_t *lines, const uint8_t *issuer, const uint8_t *currency)
{
    const int len = 24;
    uint8_t buf[len];
    buf[0] = KEYLET;
    for (int i = 0; i < 20; i++)
    {
        if (i < 3)
            buf[21 + i] = currency[i];
        buf[1 + i] = issuer[i];
    }

    write_stdout(buf, len);

    uint8_t read_buf[53];
    read(STDIN_FILENO, read_buf, sizeof(read_buf));
    uint32_t lines_len = read_buf[3] | (read_buf[2] << 8) | (read_buf[1] << 16) | (read_buf[0] << 24);

    memcpy(lines, read_buf + 4, lines_len);

    return lines_len;
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

    trace("Enter a string: ");

    read(STDIN_FILENO, buf, sizeof(buf));
    uint32_t tx_len = buf[3] | (buf[2] << 8) | (buf[1] << 16) | (buf[0] << 24);

    uint8_t tx[tx_len];
    for (int i = 0; i < tx_len; i++)
        tx[i] = buf[4 + i];

    uint8_t from_acc[20];
    for (int i = 0; i < 20; i++)
    {
        from_acc[i] = tx[i];
    }
    uint8_t to_acc[20];
    for (int i = 0; i < 20; i++)
    {
        int offset = 29;
        if (tx[21] == 0)
            offset = 52;
        to_acc[i] = tx[offset + i];
    }
    uint8_t token[3];
    token[0] = 'A';
    token[1] = 'B';
    token[2] = 'C';

    char bytes[2048];
    bin_to_hex(bytes, tx, tx_len);

    char *format = "Transaction: %s";
    int len = 13 + strlen(bytes);
    char out[len];
    sprintf(out, format, bytes);
    trace(out);

    emit(tx, sizeof(tx));

    uint8_t lines[39];
    const int lines_len = keylet(lines, from_acc, token);

    bin_to_hex(bytes, lines, lines_len);

    char *format2 = "Trustlines: %s";
    int len2 = 13 + strlen(bytes);
    char out2[len2];
    sprintf(out2, format2, bytes);
    trace(out2);

    exit(0);
}