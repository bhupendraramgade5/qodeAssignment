#ifndef PARSER_H
#define PARSER_H

#include <cstddef>
#include <cstdint>
#include "feed_handler.hpp"

struct MarketMessage;   // forward declaration

// class FeedHandler;      // forward declaration

class Parser {
public:
    explicit Parser(FeedHandler* handler);
    // void on_message(const MarketMessage& wire_msg);
    void feed(const uint8_t* data, size_t len);

private:
    void try_parse();
    void emit(MarketMessage& msg);

    FeedHandler* feed_handler_{nullptr};
    std::vector<uint8_t> buffer_;
    size_t used_{0};

    uint64_t last_sequence_{0};
};

#endif
