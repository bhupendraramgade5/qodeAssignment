#include "feed_handler.hpp"

void FeedHandler::on_message(const MarketMessage& msg) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto& entry = symbols_[msg.symbol_id];
    entry.last_msg = msg;
    entry.has_data = true;
}

bool FeedHandler::get_latest(uint16_t symbol, MarketMessage& out) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = symbols_.find(symbol);
    if (it == symbols_.end() || !it->second.has_data)
        return false;

    out = it->second.last_msg;
    return true;
}
