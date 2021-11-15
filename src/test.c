/**
 * This hook just accepts any transaction coming through it
 */
#include "../lib/hookapi.h"
#include "constants.h"
#include "evernode.h"

// Type codes.
const uint8_t STI_VL = 0X7;
const uint8_t STI_OBJECT = 0xE;

// Field codes.
const uint8_t MEMO_TYPE = 0XC;
const uint8_t MEMO_FORMAT = 0XE;
const uint8_t MEMO_DATA = 0XD;

int64_t cbak(int64_t reserved)
{
    return 0;
}

int64_t hook(int64_t reserved)
{

    uint8_t account_field[20];
    int32_t account_field_len = otxn_field(SBUF(account_field), sfAccount);

    // Memos
    // uint8_t memos[MAX_MEMO_SIZE];
    // int64_t memos_len = otxn_field(SBUF(memos), sfMemos);

    // if (!memos_len)
    //     accept(SBUF("Evernode: No memos found."), 1);

    // trace(SBUF("memos: "), memos, memos_len, 1);
    // uint8_t *memo_ptr, *type_ptr, *format_ptr, *data_ptr;
    // uint32_t memo_len, type_len, format_len, data_len;
    // GET_MEMO(0, memos, memos_len, memo_ptr, memo_len, type_ptr, type_len, format_ptr, format_len, data_ptr, data_len);

    // before we start calling hook-api functions we should tell the hook how many tx we intend to create
    etxn_reserve(1); // we are going to emit 1 transaction

    // fetch the sent Amount
    // Amounts can be 384 bits or 64 bits. If the Amount is an XRP value it will be 64 bits.
    unsigned char amount_buffer[48];
    int64_t amount_len = otxn_field(SBUF(amount_buffer), sfAmount);
    int64_t drops_to_send = AMOUNT_TO_DROPS(amount_buffer) * 2; // doubler pays back 2x received

    if (amount_len != 8)
        rollback(SBUF("Doubler: Rejecting incoming non-XRP transaction"), 5);

    int64_t fee_base = etxn_fee_base(PREPARE_PAYMENT_MEMO_SIZE);
    uint8_t tx[PREPARE_PAYMENT_MEMO_SIZE];

    // // we will use an XRP payment macro, this will populate the buffer with a serialized binary transaction
    // // Parameter list: ( buf_out, drops_amount, drops_fee, to_address, dest_tag, src_tag )
    PREPARE_PAYMENT_MEMO(tx, drops_to_send, fee_base, account_field, 0, 0);
    trace(SBUF("tx: "), tx, PREPARE_PAYMENT_MEMO_SIZE, 1);

    // // emit the transaction
    uint8_t emithash[32];
    emit(SBUF(emithash), SBUF(tx));
    accept(SBUF("Doubler: Heads, you won! Funds emitted!"), 0);

    _g(1, 1); // every hook needs to import guard function and use it at least once
    // unreachable
    return 0;
}
