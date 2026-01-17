#ifndef PARSER_H
#define PARSER_H

#include <cstddef>
#include <cstdint>
#include "exchange_simulator.hpp"

struct MarketMessage;  // still comes from exchange_simulator.hpp

enum class ParseStatus {
    OK,
    INCOMPLETE,
    MALFORMED
};

struct ParseResult {
    ParseStatus status;
    size_t bytes_consumed;
    MarketMessage message;
};

class Parser {
public:
    Parser() = default;

    // Stateless parse
    ParseResult parse(const uint8_t* data, size_t len);

private:
    uint64_t last_sequence_{0};
};

#endif
