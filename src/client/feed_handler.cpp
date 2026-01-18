#include "feed_handler.hpp"

#include "visualizer.hpp"
#include "market_data_socket.hpp"
#include "parser.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <array>
// void FeedHandler::on_message(const MarketMessage& msg) {
//     std::lock_guard<std::mutex> lock(mtx_);
//     auto& entry = symbols_[msg.symbol_id];
//     entry.last_msg = msg;
//     entry.has_data = true;
// }
FeedHandler::FeedHandler(const std::string& host, uint16_t port)
    : stream_buffer_(64 * 1024)
{
    if (!socket_.connect_to(host.c_str(), port)) {
        throw std::runtime_error("Failed to connect to exchange");
    }

    // ---- SEND SUBSCRIPTION HERE ----
    std::vector<uint16_t> symbols;
    for (uint16_t i = 1; i <=100; ++i) {   // or 500, depending on simulator
        symbols.push_back(i);
    }

    if (!socket_.send_subscription(symbols)) {
        throw std::runtime_error("Failed to send subscription");
    }

    std::cout << "[FeedHandler] Subscription sent (" 
              << symbols.size() << " symbols)\n";
}



// void FeedHandler::on_message(const MarketMessage& msg) {
//     auto& state = symbols_[msg.symbol_id];

//     uint64_t v = state.version.load(std::memory_order_relaxed);
//     state.version.store(v + 1, std::memory_order_release); // mark write start (odd)

//     state.data = msg;

//     state.version.store(v + 2, std::memory_order_release); // mark write end (even)
// }

void FeedHandler::on_message(const MarketMessage& msg) {
    if (msg.symbol_id >= MAX_SYMBOLS) {
        // Drop malformed / unexpected symbol
        return;
    }

    auto& state = symbols_[msg.symbol_id];

    uint64_t v = state.version.load(std::memory_order_relaxed);
    state.version.store(v + 1, std::memory_order_release); // write begin (odd)

    state.data = msg;
    // std::cout << "RX symbol=" << msg.symbol_id << "\n";

    state.version.store(v + 2, std::memory_order_release); // write end (even)
}


// bool FeedHandler::get_latest(uint16_t symbol, MarketMessage& out) {
//     std::lock_guard<std::mutex> lock(mtx_);
//     auto it = symbols_.find(symbol);
//     if (it == symbols_.end() || !it->second.has_data)
//         return false;

//     out = it->second.last_msg;
//     return true;
// }

// bool FeedHandler::get_latest(uint16_t symbol, MarketMessage& out) const {
//     const auto& state = symbols_[symbol];

//     while (true) {
//         uint64_t v1 = state.version.load(std::memory_order_acquire);
//         if (v1 & 1) continue; // writer in progress

//         MarketMessage tmp = state.data;

//         uint64_t v2 = state.version.load(std::memory_order_acquire);
//         if (v1 == v2) {
//             out = tmp;
//             return v2 != 0;
//         }
//     }
// }

bool FeedHandler::get_latest(uint16_t symbol, MarketMessage& out) const {
    if (symbol >= MAX_SYMBOLS) {
        return false;
    }

    const auto& state = symbols_[symbol];

    while (true) {
        uint64_t v1 = state.version.load(std::memory_order_acquire);
        if (v1 & 1) continue;

        MarketMessage tmp = state.data;

        uint64_t v2 = state.version.load(std::memory_order_acquire);
        if (v1 == v2) {
            out = tmp;
            return v2 != 0;
        }
    }
}

void FeedHandler::run() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
        perror("epoll_create1");
        return;
    }

    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = socket_.get_fd();

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_.get_fd(), &ev) < 0) {
        perror("epoll_ctl");
        return;
    }

    epoll_event events[8];

    std::cout << "[FeedHandler] Running event loop\n";

    while (true) {
        int n = epoll_wait(epoll_fd_, events, 8, -1);
        if (n < 0) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i) {
            if (events[i].events & EPOLLIN) {
                try {
                    handle_socket_read();
                } catch (const std::exception& ex) {
                    std::cerr << "[FeedHandler] " << ex.what() << "\n";
                    close(epoll_fd_);
                    return;
                }
            }
        }
    }
}

void FeedHandler::handle_socket_read() {
    uint8_t recv_buf[64 * 1024];

    while (true) {
        ssize_t bytes = socket_.recv_data(recv_buf, sizeof(recv_buf));
        if (bytes < 0) {
            // EAGAIN / EWOULDBLOCK
            break;
        }
        if (bytes == 0) {
            throw std::runtime_error("Connection closed by peer");
        }

        stream_buffer_.append(recv_buf, static_cast<size_t>(bytes));

        while (true) {
            auto result = parser_.parse(
                stream_buffer_.data_ptr(),
                stream_buffer_.data_size()
            );

            if (result.status == ParseStatus::INCOMPLETE)
                break;

            if (result.status == ParseStatus::MALFORMED) {
                seq_gaps_.fetch_add(1, std::memory_order_relaxed);
                stream_buffer_.consume(result.bytes_consumed);
                continue;
            }

            messages_.fetch_add(1, std::memory_order_relaxed);
            on_message(result.message);
            stream_buffer_.consume(result.bytes_consumed);
        }
    }
}

