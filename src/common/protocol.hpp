// #ifndef PROTOCOL_H
// #define PROTOCOL_H
// #include <cstdint>

// #pragma pack(push, 1)

// enum MessageType : uint16_t {
//     TRADE = 0x01,
//     QUOTE = 0x02,
//     HEARTBEAT = 0x03
// };

// struct MessageHeader {
//     uint16_t type;
//     uint32_t sequence;
//     uint64_t timestamp_ns;
//     uint16_t symbol_id;
// };

// struct TradePayload {
//     double price;
//     uint32_t quantity;
// };

// struct QuotePayload {
//     double bid_price;
//     uint32_t bid_qty;
//     double ask_price;
//     uint32_t ask_qty;
// };

// #pragma pack(pop)

// inline uint32_t checksum_xor(const uint8_t* data, size_t len) {
//     uint32_t c = 0;
//     for (size_t i = 0; i < len; i++) c ^= data[i];
//     return c;
// }

// #endif

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>

enum class MessageType : uint16_t {
    QUOTE = 1,
    TRADE = 2,
    HEARTBEAT = 3
};

#pragma pack(push, 1)
struct MarketMessage {
    MessageType type;
    uint16_t symbol_id;
    uint64_t sequence;
    uint64_t timestamp_ns;

    union {
        struct {
            double bid_price;
            double ask_price;
            uint32_t bid_qty;
            uint32_t ask_qty;
        } quote;

        struct {
            double trade_price;
            uint32_t trade_qty;
            uint8_t aggressor_buy;
        } trade;
    };
};
#pragma pack(pop)

#endif

