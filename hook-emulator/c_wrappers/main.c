#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>

#include "../../lib/hookmacro.h"

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
// '''''''''''''''''''''''''''''''''''''''''''''''''''''

// ' C_WRAPPER --> JS (Write to STDOUT from C_WRAPPER) '
// Trace - <TYPE:TRACE(1)><trace message>
// Transaction emit - <TYPE:EMIT(1)><account(20)><1 for xrp and 0 for iou(1)><[XRP: amount in buf(8)XFL][IOU: <issuer(20)><currency(3)><amount in buf(8)XFL>]><destination(20)><memo count(1)><[<TypeLen(1)><MemoType(20)><FormatLen(1)><MemoFormat(20)><DataLen(1)><MemoData(128)>]><ledger_hash(32)><ledger_index(8)>
// Keylet request - <TYPE:KEYLET(1)><issuer(20)><currency(3)>
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
    memcpy(out + 4, buf, len);

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

void emit_mock(const uint8_t *tx, const int len)
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

int decode_transaction(uint8_t *buffer)
{
    txn = (struct Transaction *)malloc(sizeof(struct Transaction));

    int offset = 0;

    memcpy(hook_accid, buffer, 20);
    print_bytes("Hook", hook_accid, 20);
    offset += 20;

    memcpy(txn->account, buffer + offset, 20);
    print_bytes("Account", txn->account, 20);
    offset += 20;

    if (buffer[offset] == 0b00000001)
    {
        txn->amount.is_xrp = 1;
        offset += 1;
        printf("is_xrp: true\n");
        txn->amount.xrp.amount = INT64_FROM_BUF(buffer + offset);
        print_bytes("XRP Amount", buffer + offset, 8);
        offset += 8;
        printf("XRP amount xfl: %lld\n", txn->amount.xrp.amount);

        uint64_t men = get_mantissa(txn->amount.xrp.amount);
        printf("mentissa: %lld\n", men);

        int64_t exp = get_exponent(txn->amount.xrp.amount);
        printf("exp: %lld\n", exp);
    }
    else
    {
        txn->amount.is_xrp = 0;
        offset += 1;
        printf("is_xrp: false\n");

        memcpy(txn->amount.iou.issuer, buffer + offset, 20);
        print_bytes("IOU issuer", txn->amount.iou.issuer, 20);
        offset += 20;
        memcpy(txn->amount.iou.currency, buffer + offset, 3);
        print_bytes("IOU currency", txn->amount.iou.currency, 3);
        offset += 3;
        txn->amount.iou.amount = INT64_FROM_BUF(buffer + offset);
        printf("IOU amount xfl: %lld\n", txn->amount.iou.amount);
        offset += 8;
        uint64_t men = get_mantissa(txn->amount.iou.amount);
        printf("mentissa: %lld\n", men);

        int64_t exp = get_exponent(txn->amount.iou.amount);
        printf("exp: %lld\n", exp);
    }
    memcpy(txn->destination, buffer + offset, 20);
    print_bytes("Destination", txn->destination, 20);
    offset += 20;

    txn->memos.memo_count = buffer[offset];
    printf("memo count %d\n", txn->memos.memo_count);
    offset += 1;

    txn->memos.list = (struct Memo *)malloc(sizeof(struct Memo) * txn->memos.memo_count);
    uint8_t *data_ptr = buffer + offset;
    for (size_t i = 0; i < txn->memos.memo_count; i++)
    {
        printf("memo itr %d\n", i);
        const uint8_t type_len = *data_ptr;
        printf("Type len %0d\n", type_len);
        txn->memos.list[i].type = (uint8_t *)malloc(type_len);
        memcpy(txn->memos.list[i].type, data_ptr + 1, type_len);
        print_bytes("Type", txn->memos.list[i].type, type_len);

        const uint8_t format_len = *(data_ptr + type_len + 1);
        printf("Format len %0d\n", format_len);
        if (format_len > 0)
        {
            txn->memos.list[i].format = (uint8_t *)malloc(format_len);
            memcpy(txn->memos.list[i].format, data_ptr + type_len + 2, format_len);
            print_bytes("Format", txn->memos.list[i].format, format_len);
        }
        else
        {
            txn->memos.list[i].format = NULL;
        }

        const uint8_t data_len = *(data_ptr + type_len + format_len + 2);
        printf("Data len %0d\n", data_len);
        txn->memos.list[i].data = (char *)malloc(data_len);
        memcpy(txn->memos.list[i].data, data_ptr + type_len + format_len + 3, data_len);
        print_bytes("Data", txn->memos.list[i].data, data_len);

        data_ptr += type_len + format_len + data_len + 3;
    }

    memcpy(txn->ledger_hash, data_ptr, 32);
    print_bytes("Ledger hash", txn->ledger_hash, 32);

    txn->ledger_index = UINT64_FROM_BUF(data_ptr + 32);
    printf("Ledger index: %d\n", txn->ledger_index);

    return 0;
}

int main()
{
    uint8_t buf[200000];

    // trace_out("Enter a string: ");

    // int len = read(STDIN_FILENO, buf, sizeof(buf));
    const char *path = "/home/savinda/Documents/HotPocketDev/evernode-hook/hook-emulator/c_wrappers/tx_out.bin";
    const int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        printf("Error opening file\n");
        return 1;
    }
    const int ret = read(fd, buf,sizeof(buf));
    close(fd);
    // printf("%d\n", ret);
    // print_bytes("data", buf, ret);

    // const uint32_t read_len = buf[3] | (buf[2] << 8) | (buf[1] << 16) | (buf[0] << 24);
    // const char *path_write = "/home/savinda/Documents/HotPocketDev/evernode-hook/hook-emulator/c_wrappers/tx_out.bin";
    // const int fd_write = open(path_write, O_WRONLY | O_CREAT);
    // write(fd_write, buf + 4, read_len);
    // close(fd_write);

    // decode_transaction(buf + 4);
    decode_transaction(buf);

    // const uint32_t tx_len = read_len - 20;

    // uint8_t hook_accid[20];
    // uint8_t tx[tx_len];
    // memcpy(hook_accid, &buf[4], 20);
    // memcpy(tx, &buf[24], tx_len);

    // uint8_t from_acc[20];
    // memcpy(from_acc, tx, 20);
    // uint8_t to_acc[20];
    // if (tx[21] == 0)
    //     memcpy(to_acc, tx, 52);
    // else
    //     memcpy(to_acc, tx, 29);

    // uint8_t token[3];
    // token[0] = 'A';
    // token[1] = 'B';
    // token[2] = 'C';

    // char bytes[2048];
    // bin_to_hex(bytes, hook_accid, 20);
    // char *format = "Hook: %s";
    // int len = 13 + strlen(bytes);
    // char out[len];
    // sprintf(out, format, bytes);
    // trace_out(out);

    // bin_to_hex(bytes, tx, tx_len);
    // format = "Transaction: %s";
    // len = 13 + strlen(bytes);
    // char out2[len];
    // sprintf(out2, format, bytes);
    // trace_out(out2);

    // emit_mock(tx, sizeof(tx));

    // uint8_t lines[39];
    // const int lines_len = keylet(lines, from_acc, token);

    // bin_to_hex(bytes, lines, lines_len);
    // format = "Trustlines: %s";
    // len = 13 + strlen(bytes);
    // char out3[len];
    // sprintf(out3, format, bytes);
    // trace_out(out3);

    // int64_t s = otxn_slot(0);
    // printf("Slot: %lld\n", s);

    // int64_t amt = slot_subfield(s, sfAmount, 0);

    // uint8_t amount_buf[48];
    // slot(amount_buf, sizeof(amount_buf), amt);
    // print_bytes("Amount", amount_buf, 48);

    // uint8_t hook[20];
    // hook_account(SBUF(hook));
    // // printf("amt: %lld\n", amt);
    // print_bytes("hook", SBUF(hook));
    // hook(0);

    int64_t f1 = float_set(-5, 1);
    // f1 = float_negate(f1);

    int64_t f2 = float_set(-5, 1);
    // f2 = float_negate(f2);

    int64_t ans = float_compare(f1, f2, COMPARE_GREATER);

    // int64_t out = float_int(ans, 6, 1);
    // printf("is_neg_ans: %d\n", is_negative(ans));
    printf("Ans: %lld\n", ans);

    FREE_TRANSACTION_OBJ

    exit(0);
}