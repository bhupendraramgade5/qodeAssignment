// #include "parser.h"
// #include "protocol.h"

// #include <atomic>
// #include <cstring>
// #include <iostream>

// std::atomic<uint64_t> g_messages{0};
// std::atomic<uint64_t> g_seq_gaps{0};

// void Parser::feed(const uint8_t* data, size_t len) {
//     static uint8_t buffer[8192];
//     static size_t used = 0;
//     static uint32_t last_seq = 0;

//     memcpy(buffer + used, data, len);
//     used += len;

//     size_t offset = 0;
//     while (used - offset >= sizeof(MessageHeader) + 4) {
//         auto* hdr = (MessageHeader*)(buffer + offset);

//         size_t msg_len =
//             sizeof(MessageHeader) +
//             (hdr->type == TRADE ? sizeof(TradePayload) :
//              hdr->type == QUOTE ? sizeof(QuotePayload) : 0) +
//             4;

//         if (used - offset < msg_len) break;

//         if (last_seq && hdr->sequence != last_seq + 1)
//             g_seq_gaps++;

//         last_seq = hdr->sequence;
//         g_messages++;

//         offset += msg_len;
//     }

//     memmove(buffer, buffer + offset, used - offset);
//     used -= offset;
// }

// void Parser::emit(MarketMessage& msg) {
//     // integers
//     msg.symbol_id    = ntohs(msg.symbol_id);
//     msg.sequence     = be64toh(msg.sequence);
//     msg.timestamp_ns = be64toh(msg.timestamp_ns);

//     // doubles (if you encode them)
//     if (msg.type == MessageType::QUOTE) {
//         msg.quote.bid_price = ntohd(*(uint64_t*)&msg.quote.bid_price);
//         msg.quote.ask_price = ntohd(*(uint64_t*)&msg.quote.ask_price);
//     } else {
//         msg.trade.trade_price = ntohd(*(uint64_t*)&msg.trade.trade_price);
//     }

//     feed_handler_->on_message(msg);
// }


#include "parser.h"
#include "../common/protocol.h"   // MarketMessage
#include "exchange_simulator.hpp"
#include <cstring>
#include <arpa/inet.h>

// Parser::Parser(FeedHandler* handler)
//     : feed_handler_(handler) {}

// void Parser::feed(const uint8_t* data, size_t len) {
//     // Append incoming bytes
//     std::memcpy(buffer_ + used_, data, len);
//     used_ += len;

//     size_t offset = 0;

//     while (used_ - offset >= sizeof(MarketMessage)) {
//         MarketMessage msg;
//         std::memcpy(&msg, buffer_ + offset, sizeof(MarketMessage));

//         emit(msg);
//         offset += sizeof(MarketMessage);
//     }

//     // Shift remaining bytes
//     std::memmove(buffer_, buffer_ + offset, used_ - offset);
//     used_ -= offset;
// }

std::atomic<uint64_t> g_messages{0};
std::atomic<uint64_t> g_seq_gaps{0};


Parser::Parser(FeedHandler* handler)
    : feed_handler_(handler) {
    buffer_.reserve(8192);
}

void Parser::feed(const uint8_t* data, size_t len) {
    buffer_.insert(buffer_.end(), data, data + len);
    try_parse();
}

void Parser::emit(MarketMessage& msg) {
    // ---- endian conversion ----
    msg.symbol_id    = ntohs(msg.symbol_id);
    msg.sequence     = be64toh(msg.sequence);
    msg.timestamp_ns = be64toh(msg.timestamp_ns);

    // Sequence gap detection
    if (last_sequence_ && msg.sequence != last_sequence_ + 1) {
        // report gap
    }
    last_sequence_ = msg.sequence;

    // IMPORTANT: doubles are NOT endian-safe
    // If you want true portability, you MUST encode them as uint64_t
    // For now assume same-endian machines

    feed_handler_->on_message(msg);
}

// void Parser::try_parse() {
//     size_t offset = 0;

//     while (buffer_.size() - offset >= sizeof(MarketMessage)) {
//         MarketMessage msg;
//         std::memcpy(&msg, buffer_.data() + offset, sizeof(msg));

//         // endian fix
//         msg.symbol_id    = ntohs(msg.symbol_id);
//         msg.sequence     = be64toh(msg.sequence);
//         msg.timestamp_ns = be64toh(msg.timestamp_ns);

//         // sequence gap detection
//         if (last_sequence_ && msg.sequence != last_sequence_ + 1) {
//             // increment gap counter
//         }
//         last_sequence_ = msg.sequence;

//         feed_handler_->on_message(msg);

//         offset += sizeof(MarketMessage);
//     }

//     buffer_.erase(buffer_.begin(), buffer_.begin() + offset);
// }

void Parser::try_parse() {
    size_t offset = 0;

    while (used_ - offset >= sizeof(MarketMessage)) {
        MarketMessage msg;
        std::memcpy(&msg, buffer_.data() + offset, sizeof(MarketMessage));

        // Endian conversion
        msg.symbol_id    = ntohs(msg.symbol_id);
        msg.sequence     = be64toh(msg.sequence);
        msg.timestamp_ns = be64toh(msg.timestamp_ns);

        if (last_sequence_ && msg.sequence != last_sequence_ + 1)
            g_seq_gaps++;

        last_sequence_ = msg.sequence;
        g_messages++;

        feed_handler_->on_message(msg);

        offset += sizeof(MarketMessage);
    }

    if (offset > 0) {
        std::memmove(buffer_.data(),
                     buffer_.data() + offset,
                     used_ - offset);
        used_ -= offset;
    }
}