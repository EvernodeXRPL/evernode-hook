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

// Transaction - <account(20)><1 for xrp and 0 for iou(1)><[XRP: amount in buf(8)XFL][IOU: <issuer(20)><currency(3)><amount in buf(8)XFL>]><destination(20)><memo count(1)><[<TypeLen(1)><MemoType(20)><FormatLen(1)><MemoFormat(20)><DataLen(1)><MemoData(128)>]><ledger_hash(32)><ledger_index(8)>

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

void keylet(const char *address, const char *issuer, const char *currency)
{
    const int len = 1 + strlen(address) + strlen(issuer) + strlen(currency);
    char buf[len];
    buf[0] = KEYLET;
    sprintf(&buf[1], "%s%s%s\n", address, issuer, currency);
    write_stdout(buf, len);
}

int main()
{
    uint8_t buf[200000];

    trace("Enter a string: ");

    read(STDIN_FILENO, buf, sizeof(buf));
    uint32_t txLen = buf[3] | (buf[2] << 8) | (buf[1] << 16) | (buf[0] << 24);

    uint8_t tx[txLen];
    for (int i = 0; i < txLen; i++)
        tx[i] = buf[4 + i];

    char bytes[2048];
    strcpy(bytes, "");
    for (int i = 0; i < txLen; i++)
    {
        char n[20];
        sprintf(n, "%02X", tx[i]);
        strcat(bytes, n);
    }
    strcat(bytes, "\0");

    char *format = "Transaction: %s";
    int len = 13 + strlen(bytes);
    char out[len];
    sprintf(out, format, bytes);
    trace(out);

    emit(tx, sizeof(tx));

    keylet("rqiGkSMnQbBiC1to5hs3G34iAgLt5PG7D", "rUsJzadhnCbnjjUXQCjzCUdC7CbBtXhJ3P", "ABC");

    fgets(bytes, sizeof(bytes), stdin);

    /* remove newline, if present */
    int i = strlen(bytes) - 1;
    if (bytes[i] == '\n')
        bytes[i] = '\0';

    format = "Trustlines: %s";
    len = 13 + i;
    char out2[len];
    sprintf(out2, format, bytes);
    trace(out2);

    exit(0);
}