#include "parser.hpp"
// #include "exchange_simulator.hpp"
#include <cstring>
#include <arpa/inet.h>

ParseResult Parser::parse(const uint8_t* data, size_t len) {
    ParseResult result{};

    // Not enough data for even one message
    if (len < sizeof(MarketMessage)) {
        result.status = ParseStatus::INCOMPLETE;
        result.bytes_consumed = 0;
        return result;
    }

    MarketMessage msg;
    std::memcpy(&msg, data, sizeof(MarketMessage));

    // ---- endian conversion ----
    msg.symbol_id    = ntohs(msg.symbol_id);
    msg.sequence     = be64toh(msg.sequence);
    msg.timestamp_ns = be64toh(msg.timestamp_ns);

    // Sequence gap detection (report, not act)
    if (last_sequence_ && msg.sequence != last_sequence_ + 1) {
        // just detect for now
        // feed handler decides what to do later
    }
    last_sequence_ = msg.sequence;

    result.status = ParseStatus::OK;
    result.bytes_consumed = sizeof(MarketMessage);
    result.message = msg;
    return result;
}
