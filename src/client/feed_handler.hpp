#ifndef FEE_HANDLER_H
#define FEE_HANDLER_H
#include <unordered_map>
#include <vector>
#include <mutex>
#include <cstdint>
#include "parser.h"
#include "exchange_simulator.hpp"

struct SymbolSnapshot {
    MarketMessage last_msg;
    bool has_data = false;
};

class FeedHandler {
public:
    void on_message(const MarketMessage& msg);

    bool get_latest(uint16_t symbol, MarketMessage& out);

private:
    std::unordered_map<uint16_t, SymbolSnapshot> symbols_;
    std::mutex mtx_;
};

#endif