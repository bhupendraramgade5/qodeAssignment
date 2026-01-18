// #ifndef MARKET_DATA_H
// #define MARKET_DATA_H
// #include <cstdint>
// #include <cstddef>
// #include <sys/types.h>   // ✅ for ssize_t

// class MarketDataSocket {
// public:
//     bool connect_to(const char* host, uint16_t port);
//     ssize_t recv_data(void* buf, size_t len);
//     int get_fd() const;
//     int run();
// private:
//     int fd{-1};
// };
// #endif


#ifndef MARKET_DATA_SOCKET_H
#define MARKET_DATA_SOCKET_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <sys/types.h>   // ssize_t

class MarketDataSocket {
public:
    MarketDataSocket() = default;
    ~MarketDataSocket();

    bool send_subscription(const std::vector<uint16_t>& symbols);

    bool connect_to(const char* host, uint16_t port);
    ssize_t recv_data(void* buf, size_t len);
    ssize_t send_data(const void* buf, size_t len);

    int get_fd() const;
    void close();

private:
    int fd_{-1};
};

#endif // MARKET_DATA_SOCKET_H
