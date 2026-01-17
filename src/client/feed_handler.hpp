#ifndef FEED_HANDLER_H
#define FEED_HANDLER_H

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <atomic>

#include "parser.h"
#include "protocol.h"
#include "market_data_socket.h"

// struct SymbolSnapshot {
//     MarketMessage last_msg;
//     bool has_data{false};
// };

struct alignas(64) SymbolState {
    std::atomic<uint64_t> version{0};
    MarketMessage data;
};


struct StreamBuffer {
    std::vector<uint8_t> buffer;
    size_t offset{0};

    StreamBuffer() {
        buffer.reserve(64 * 1024);
    }

    void append(const uint8_t* data, size_t len) {
        buffer.insert(buffer.end(), data, data + len);
    }

    const uint8_t* data_ptr() const {
        return buffer.data() + offset;
    }

    size_t data_size() const {
        return buffer.size() - offset;
    }

    void consume(size_t n) {
        offset += n;

        if (offset > 0 && offset >= buffer.size() / 2) {
            buffer.erase(buffer.begin(), buffer.begin() + offset);
            offset = 0;
        }
    }
};

class FeedHandler {
public:
    FeedHandler(const std::string& host, uint16_t port);

    void run();   // main event loop

    bool get_latest(uint16_t symbol, MarketMessage& out) const;
    // std::mutex mtx_;
private:
    // network
    MarketDataSocket socket_;
    int epoll_fd_{-1};

    // parsing
    Parser parser_;
    StreamBuffer stream_buffer_;

    // state
    // std::unordered_map<uint16_t, SymbolSnapshot> symbols_;
    static constexpr size_t MAX_SYMBOLS = 1024;
    std::array<SymbolState, MAX_SYMBOLS> symbols_;

    // stats
    std::atomic<uint64_t> messages_{0};
    std::atomic<uint64_t> seq_gaps_{0};

    void on_message(const MarketMessage& msg);
    void handle_socket_read();
    
    bool get_latest(uint16_t symbol, MarketMessage& out) ;
};

#endif
