#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>

#include "../../lib/hookmacro.h"

#define MAX_READ_LEN 1024

// --------------- Message data formats ----------------

// ''''' JS --> C_WRAPPER (Write to STDIN from JS) '''''
// Transaction origin - <hookid(20)><account(20)><1 for xrp and 0 for iou(1)><[XRP: amount in buf(8)XFL][IOU: <issuer(20)><currency(3)><amount in buf(8)XFL>]><destination(20)><memo count(1)><[<TypeLen(1)><MemoType(20)><FormatLen(1)><MemoFormat(20)><DataLen(1)><MemoData(128)>]><ledger_hash(32)><ledger_index(8)>
// Message response format - <RETURN CODE(1)><DATA>
// Emit response - <return code(1)><tx hash(32)>
// Trustline response - <return code(1)><balance(8)XFL><limit(8)XFL>
// State get response - <return code(1)><value(128)>
// State set response - <return code(1)>
// Accid response - <return code(1)><account_id(20)>
// '''''''''''''''''''''''''''''''''''''''''''''''''''''

// ' C_WRAPPER --> JS (Write to STDOUT from C_WRAPPER) '
// Message protocol - <data len(4)><data>
// Message request format - <TYPE(1)><DATA>
// Trace request - <type:TRACE(1)><trace message>
// Transaction emit request - <type:EMIT(1)><prepared transaction buf>
// Trustline request - <type:TRUSTLINE(1)><address(20)><issuer(20)><currency(3)>
// State set request - <type:STATE_GET(1)><key(32)><value(128)>
// State get request - <type:STATE_SET(1)><key(32)>
// Accid request - <type:ACCID(1)><raddress(35)>
// '''''''''''''''''''''''''''''''''''''''''''''''''''''
// -----------------------------------------------------
/**
 * Decode the transaction buffer sent from JS and populate the struct.
 * @param buffer - The transaction buffer sent from JS.
*/
int decode_transaction(uint8_t *buffer)
{
    txn = (struct Transaction *)malloc(sizeof(struct Transaction));

    int offset = 0;

    memcpy(hook_accid, buffer, 20);
    offset += 20;

    memcpy(txn->account, buffer + offset, 20);
    offset += 20;

    if (buffer[offset] == 0b00000001)
    {
        txn->amount.is_xrp = 1;
        offset += 1;
        txn->amount.xrp.amount = INT64_FROM_BUF(buffer + offset);
        offset += 8;
    }
    else
    {
        txn->amount.is_xrp = 0;
        offset += 1;

        memcpy(txn->amount.iou.issuer, buffer + offset, 20);
        offset += 20;
        memset(txn->amount.iou.currency, 0, 20);
        memcpy(txn->amount.iou.currency + 12, buffer + offset, 3);
        offset += 3;
        txn->amount.iou.amount = INT64_FROM_BUF(buffer + offset);
        offset += 8;
    }
    memcpy(txn->destination, buffer + offset, 20);
    offset += 20;

    txn->memos.memo_count = buffer[offset];
    offset += 1;

    txn->memos.list = (struct Memo *)malloc(sizeof(struct Memo) * txn->memos.memo_count);
    uint8_t *data_ptr = buffer + offset;
    for (size_t i = 0; i < txn->memos.memo_count; i++)
    {
        const uint8_t type_len = *data_ptr;
        if (type_len > 0)
        {
            txn->memos.list[i].type = (uint8_t *)malloc(type_len);
            memcpy(txn->memos.list[i].type, data_ptr + 1, type_len);
            txn->memos.list[i].type_len = type_len;
        }
        else
        {
            txn->memos.list[i].type = NULL;
        }

        const uint8_t format_len = *(data_ptr + type_len + 1);
        if (format_len > 0)
        {
            txn->memos.list[i].format = (uint8_t *)malloc(format_len);
            memcpy(txn->memos.list[i].format, data_ptr + type_len + 2, format_len);
            txn->memos.list[i].format_len = format_len;
        }
        else
        {
            txn->memos.list[i].format = NULL;
        }

        const uint8_t data_len = *(data_ptr + type_len + format_len + 2);
        if (data_len > 0)
        {
            txn->memos.list[i].data = (char *)malloc(data_len);
            memcpy(txn->memos.list[i].data, data_ptr + type_len + format_len + 3, data_len);
            txn->memos.list[i].data_len = data_len;
        }
        else
        {
            txn->memos.list[i].data = NULL;
        }

        data_ptr += type_len + format_len + data_len + 3;
    }

    memcpy(txn->ledger_hash, data_ptr, 32);
    txn->ledger_index = UINT64_FROM_BUF(data_ptr + 32);

    return 0;
}

int main()
{
    // Read the transaction from STDIN.
    uint8_t data_buf[MAX_READ_LEN];
    const int data_len = read_stdin(data_buf, sizeof(data_buf));

    // Decode the transaction and populate the struct.
    decode_transaction(data_buf);

    // Invoke the hook logic.
    hook(0);

    // Clear any memory allocations made for the transaction struct and slots.
    CLEAN_EMULATOR

    return 0;
}