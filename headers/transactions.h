#ifndef TRANSACTIONS_INCLUDED
#define TRANSACTIONS_INCLUDED 1

/**************************************************************************/
/*************Pre-populated templates of Payment Transactions**************/
/**************************************************************************/

// IOU Payment with single memo (Reward).
uint8_t REWARD_PAYMENT[333] = {
    0x12, 0x00, 0x00,                   // transaction_type(ttPAYMENT)
    0x22, 0x80, 0x00, 0x00, 0x00,       // flags(tfCANONICAL)
    0x23, 0x00, 0x00, 0x00, 0x00,       // TAG_SOURCE
    0x24, 0x00, 0x00, 0x00, 0x00,       // sequence(0)
    0x2E, 0x00, 0x00, 0x00, 0x00,       // TAG DESTINATION
    0x20, 0x1A, 0x00, 0x00, 0x00, 0x00, // first_ledger_sequence(ledger_seq + 1) - Added on prepare to offset 25
    0x20, 0x1B, 0x00, 0x00, 0x00, 0x00, // last_ledger_sequence(ledger_seq + 5) - Added on prepare to offset 31
    0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // amount(<type(1)><amount(8)><currency_code(20)><issuer(20)>) - Added on prepare to offset 36
    0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // fee Added on prepare to offset 84
    0x73, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Signing Public Key (NULL offset 95)
    0x81, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_source(20) - Added on prepare to offset 130
    0x83, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account_destination(20) - Added on prepare to offset 152
    0xF9, 0xEA, // Memo array and object start markers
    0x7C, 0x0D,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // MemoType (13 bytes) offset 176
    0x7D, 0x00,
    0x7E, 0x00,
    0xE1, 0xF1, // Memo array and object end markers
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit_details(138) - Added on prepare to offset 191
    // emit_details - NOTE : Considered additional 22 bytes for the callback scenario.
};

// Set Hook
uint8_t SET_HOOK[465] = {
    0x12, 0x00, 0x16,                                     // transaction_type(ttHOOK_SET)
    0x22, 0x00, 0x00, 0x00, 0x02,                         // flags (tfOnlyXRP)
    0x24, 0x00, 0x00, 0x00, 0x00,                         // sequence(0)
    0x20, 0x1A, 0x00, 0x00, 0x00, 0x00,                   // first_ledger_sequence(ledger_seq + 1) - Added on prepare to offset 15
    0x20, 0x1B, 0x00, 0x00, 0x00, 0x00,                   // last_ledger_sequence(ledger_seq + 5) - Added on prepare to offset 21
    0x68, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // fee - Added on prepare to offset 25
    0x73, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // signing pubkey(NULL)
    0x81, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // account - Added on prepare to offset 71
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, // hook objects - Added on prepare to offset 91
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // emit details(138)
};

#define PAYMENT_TRUSTLINE_TX_SIZE \
    sizeof(PAYMENT_TRUSTLINE)
#define PREPARE_PAYMENT_TRUSTLINE_TX(token, issuer, amount, to_address)  \
    {                                                                    \
        uint8_t *buf_out = PAYMENT_TRUSTLINE;                            \
        uint32_t cls = (uint32_t)ledger_seq();                           \
        UINT32_TO_BUF((buf_out + 25), cls + 1);                          \
        UINT32_TO_BUF((buf_out + 31), cls + 5);                          \
        SET_AMOUNT_OUT((buf_out + 36), token, issuer, amount);           \
        COPY_20BYTES((buf_out + 130), hook_accid);                       \
        COPY_20BYTES((buf_out + 152), to_address);                       \
        etxn_details((buf_out + 172), PAYMENT_TRUSTLINE_TX_SIZE);        \
        int64_t fee = etxn_fee_base(buf_out, PAYMENT_TRUSTLINE_TX_SIZE); \
        uint8_t *fee_ptr = buf_out + 84;                                 \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee);                    \
    }

#define PRUNED_HOST_REBATE_COMMON(buf_out, to_address)                \
    {                                                                 \
        COPY_20BYTES((buf_out), hook_accid);                          \
        COPY_20BYTES((buf_out + 22), to_address);                     \
        COPY_16BYTES((buf_out + 46), DEAD_HOST_PRUNE_REF);            \
        COPY_2BYTES((buf_out + 46 + 16), (DEAD_HOST_PRUNE_REF + 16)); \
        COPY_BYTE((buf_out + 46 + 18), (DEAD_HOST_PRUNE_REF + 18));   \
        COPY_20BYTES((buf_out + 67), PRUNE_MESSAGE);                  \
        COPY_8BYTES((buf_out + 89), FORMAT_TEXT);                     \
        COPY_2BYTES((buf_out + 89 + 8), (FORMAT_TEXT + 8));           \
    }

#define PRUNED_HOST_REBATE_MIN_PAYMENT_TX_SIZE \
    sizeof(PRUNED_HOST_REBATE_MIN_PAYMENT)
#define PREPARE_PRUNED_HOST_REBATE_MIN_PAYMENT_TX(drops_amount, to_address)           \
    {                                                                                 \
        uint8_t *buf_out = PRUNED_HOST_REBATE_MIN_PAYMENT;                            \
        uint32_t cls = (uint32_t)ledger_seq();                                        \
        UINT32_TO_BUF((buf_out + 25), cls + 1);                                       \
        UINT32_TO_BUF((buf_out + 31), cls + 5);                                       \
        uint8_t *buf_ptr = (buf_out + 35);                                            \
        _06_01_ENCODE_DROPS_AMOUNT(buf_ptr, drops_amount);                            \
        PRUNED_HOST_REBATE_COMMON((buf_out + 90), to_address);                        \
        etxn_details((buf_out + 191), PRUNED_HOST_REBATE_MIN_PAYMENT_TX_SIZE);        \
        int64_t fee = etxn_fee_base(buf_out, PRUNED_HOST_REBATE_MIN_PAYMENT_TX_SIZE); \
        buf_ptr = buf_out + 44;                                                       \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(buf_ptr, fee);                                 \
    }

#define PRUNED_HOST_REBATE_PAYMENT_TX_SIZE \
    sizeof(PRUNED_HOST_REBATE_PAYMENT)
#define PREPARE_PRUNED_HOST_REBATE_PAYMENT_TX(evr_amount, to_address)             \
    {                                                                             \
        uint8_t *buf_out = PRUNED_HOST_REBATE_PAYMENT;                            \
        uint32_t cls = (uint32_t)ledger_seq();                                    \
        UINT32_TO_BUF((buf_out + 25), cls + 1);                                   \
        UINT32_TO_BUF((buf_out + 31), cls + 5);                                   \
        SET_AMOUNT_OUT((buf_out + 36), EVR_TOKEN, issuer_accid, evr_amount);      \
        PRUNED_HOST_REBATE_COMMON((buf_out + 130), to_address);                   \
        etxn_details((buf_out + 231), PRUNED_HOST_REBATE_PAYMENT_TX_SIZE);        \
        int64_t fee = etxn_fee_base(buf_out, PRUNED_HOST_REBATE_PAYMENT_TX_SIZE); \
        uint8_t *fee_ptr = buf_out + 84;                                          \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee);                             \
    }

#define REG_NFT_MINT_TX_SIZE \
    sizeof(REG_NFT_MINT_TX)
#define PREPARE_REG_NFT_MINT_TX(txid)                               \
    {                                                               \
        uint8_t *buf_out = REG_NFT_MINT_TX;                         \
        UINT32_TO_BUF((buf_out + 18), cur_ledger_seq + 1);          \
        UINT32_TO_BUF((buf_out + 24), cur_ledger_seq + 5);          \
        COPY_32BYTES((buf_out + 87), txid);                         \
        COPY_20BYTES((buf_out + 121), hook_accid);                  \
        etxn_details((buf_out + 141), 138);                         \
        int64_t fee = etxn_fee_base(buf_out, REG_NFT_MINT_TX_SIZE); \
        uint8_t *fee_ptr = buf_out + 34;                            \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee);               \
    }

#define NFT_BURN_TX_SIZE \
    sizeof(NFT_BURN_TX)
#define PREPARE_NFT_BURN_TX(nftid, owner)                       \
    {                                                           \
        uint8_t *buf_out = NFT_BURN_TX;                         \
        UINT32_TO_BUF((buf_out + 15), cur_ledger_seq + 1);      \
        UINT32_TO_BUF((buf_out + 21), cur_ledger_seq + 5);      \
        COPY_32BYTES((buf_out + 26), nftid);                    \
        COPY_20BYTES((buf_out + 104), hook_accid);              \
        COPY_20BYTES((buf_out + 126), owner);                   \
        etxn_details((buf_out + 146), 138);                     \
        int64_t fee = etxn_fee_base(buf_out, NFT_BURN_TX_SIZE); \
        uint8_t *fee_ptr = buf_out + 58;                        \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee);           \
    }

#define NFT_BUY_OFFER_TRUSTLINE_TX_SIZE \
    sizeof(NFT_BUY_OFFER_TRUSTLINE)
#define PREPARE_NFT_BUY_OFFER_TRUSTLINE_TX(token, issuer, amount, to_address, tknid) \
    {                                                                                \
        uint8_t *buf_out = NFT_BUY_OFFER_TRUSTLINE;                                  \
        UINT32_TO_BUF((buf_out + 20), cur_ledger_seq + 1);                           \
        UINT32_TO_BUF((buf_out + 26), cur_ledger_seq + 5);                           \
        SET_AMOUNT_OUT((buf_out + 31), token, issuer, amount);                       \
        COPY_32BYTES((buf_out + 80), tknid)                                          \
        COPY_20BYTES((buf_out + 158), hook_accid);                                   \
        COPY_20BYTES((buf_out + 180), to_address);                                   \
        etxn_details((uint32_t)buf_out + 200, 138);                                  \
        int64_t fee = etxn_fee_base(buf_out, NFT_BUY_OFFER_TRUSTLINE_TX_SIZE);       \
        uint8_t *fee_ptr = buf_out + 112;                                            \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(fee_ptr, fee);                                \
    }

#define NFT_OFFER_TX_SIZE \
    sizeof(NFT_OFFER)
#define PREPARE_NFT_BUY_OFFER_TX(drops_amount_raw, to_address, tknid) \
    {                                                                 \
        uint8_t *buf_out = NFT_OFFER;                                 \
        UINT32_TO_BUF((buf_out + 4), tfBuyToken);                     \
        UINT32_TO_BUF((buf_out + 20), cur_ledger_seq + 1);            \
        UINT32_TO_BUF((buf_out + 26), cur_ledger_seq + 5);            \
        uint8_t *buf_ptr = buf_out + 30;                              \
        _06_01_ENCODE_DROPS_AMOUNT(buf_ptr, drops_amount_raw);        \
        COPY_32BYTES((buf_out + 40), tknid)                           \
        COPY_20BYTES((buf_out + 118), hook_accid);                    \
        *(buf_out + 138) = 0x82;                                      \
        COPY_20BYTES((buf_out + 140), to_address);                    \
        etxn_details((uint32_t)buf_out + 160, 138);                   \
        int64_t fee = etxn_fee_base(buf_out, NFT_OFFER_TX_SIZE);      \
        buf_ptr = buf_out + 72;                                       \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(buf_ptr, fee);                 \
    }

#define PREPARE_NFT_SELL_OFFER_TX(drops_amount_raw, to_address, tknid) \
    {                                                                  \
        uint8_t *buf_out = NFT_OFFER;                                  \
        UINT32_TO_BUF((buf_out + 4), tfSellToken);                     \
        UINT32_TO_BUF((buf_out + 20), cur_ledger_seq + 1);             \
        UINT32_TO_BUF((buf_out + 26), cur_ledger_seq + 5);             \
        uint8_t *buf_ptr = buf_out + 30;                               \
        _06_01_ENCODE_DROPS_AMOUNT(buf_ptr, drops_amount_raw);         \
        COPY_32BYTES((buf_out + 40), tknid)                            \
        COPY_20BYTES((buf_out + 118), hook_accid);                     \
        *(buf_out + 138) = 0x83;                                       \
        COPY_20BYTES((buf_out + 140), to_address);                     \
        etxn_details((uint32_t)buf_out + 160, 138);                    \
        int64_t fee = etxn_fee_base(buf_out, NFT_OFFER_TX_SIZE);       \
        buf_ptr = buf_out + 72;                                        \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(buf_ptr, fee);                  \
    }

#define ENCODE_HASH_SIZE 34
#define HOOK_HASH 0x1FU
#define HOOK_NAMESPACE 0x20U
#define ENCODE_HASH(buf_out, hash_ptr, t)                           \
    {                                                               \
        buf_out[0] = 0x50U + 0U;                                    \
        buf_out[1] = t;                                             \
        *(uint64_t *)(buf_out + 2) = *(uint64_t *)(hash_ptr + 0);   \
        *(uint64_t *)(buf_out + 10) = *(uint64_t *)(hash_ptr + 8);  \
        *(uint64_t *)(buf_out + 18) = *(uint64_t *)(hash_ptr + 16); \
        *(uint64_t *)(buf_out + 26) = *(uint64_t *)(hash_ptr + 24); \
        buf_out += ENCODE_HASH_SIZE;                                \
    }

#define _05_31_ENCODE_HOOK_HASH(buf_out, hash_ptr) \
    ENCODE_HASH(buf_out, hash_ptr, HOOK_HASH);

#define _05_32_ENCODE_HOOK_NAMESPACE(buf_out, hash_ptr) \
    ENCODE_HASH(buf_out, hash_ptr, HOOK_NAMESPACE);

#define ENCODE_DELETEHOOK_SIZE 2
#define ENCODE_DELETEHOOK(buf_out)         \
    {                                      \
        buf_out[0] = 0x70U + 0xBU;         \
        buf_out[1] = 0x00U;                \
        buf_out += ENCODE_DELETEHOOK_SIZE; \
    }

#define _07_11_ENCODE_DELETE_HOOK(buf_out) \
    ENCODE_DELETEHOOK(buf_out);

/**************************************************************************/
/**************************MEMO related MACROS*****************************/
/**************************************************************************/

#define ENCODE_FIELDS_SIZE 1U
#define ENCODE_FIELDS(buf_out, type, field) \
    {                                       \
        uint8_t uf = field;                 \
        buf_out[0] = type + (uf & 0x0FU);   \
        buf_out += ENCODE_FIELDS_SIZE;      \
    }

#define ENCODE_HOOK_OBJECT(buf_out, hash, namespace)                                   \
    {                                                                                  \
        ENCODE_FIELDS(buf_out, OBJECT, HOOK); /*Obj start*/ /* uint32  | size   1 */   \
        _02_02_ENCODE_FLAGS(buf_out, tfHookOveride);        /* uint32  | size   5 */   \
        if (!IS_BUFFER_EMPTY_32(hash))                                                 \
        {                                                                              \
            _05_31_ENCODE_HOOK_HASH(buf_out, hash);           /* uint256 | size  34 */ \
            _05_32_ENCODE_HOOK_NAMESPACE(buf_out, namespace); /* uint256 | size  34 */ \
        }                                                                              \
        else                                                                           \
        {                                                                              \
            _07_11_ENCODE_DELETE_HOOK(buf_out); /* blob    | size   2 */               \
        }                                                                              \
        ENCODE_FIELDS(buf_out, OBJECT, END); /*Arr End*/ /* uint32  | size   1 */      \
    }

#define PREPARE_SET_HOOK_TX(hash_arr, namespace, tx_size)                     \
    {                                                                         \
        uint8_t *buf_out = SET_HOOK;                                          \
        UINT32_TO_BUF((buf_out + 15), cur_ledger_seq + 1);                    \
        UINT32_TO_BUF((buf_out + 21), cur_ledger_seq + 5);                    \
        COPY_20BYTES((buf_out + 71), hook_accid);                             \
        uint8_t *cur_ptr = buf_out + 91;                                      \
        ENCODE_FIELDS(cur_ptr, ARRAY, HOOKS);                                 \
        ENCODE_HOOK_OBJECT(cur_ptr, hash_arr, namespace);                     \
        ENCODE_HOOK_OBJECT(cur_ptr, (hash_arr + HASH_SIZE), namespace);       \
        ENCODE_HOOK_OBJECT(cur_ptr, (hash_arr + (2 * HASH_SIZE)), namespace); \
        ENCODE_HOOK_OBJECT(cur_ptr, (hash_arr + (3 * HASH_SIZE)), namespace); \
        ENCODE_FIELDS(cur_ptr, ARRAY, END);                                   \
        tx_size = cur_ptr - buf_out + 138;                                    \
        etxn_details(cur_ptr, 138);                                           \
        int64_t fee = etxn_fee_base(buf_out, tx_size);                        \
        cur_ptr = SET_HOOK + 25;                                              \
        CHECK_AND_ENCODE_FINAL_TRX_FEE(cur_ptr, fee);                         \
    }

#endif
