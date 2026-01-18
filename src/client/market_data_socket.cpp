// #include "market_data_socket.h"

// #include <arpa/inet.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <netinet/tcp.h>
// #include <cstring>
// #include <cerrno>     // ✅ for errno, EINPROGRESS

// bool MarketDataSocket::connect_to(const char* host, uint16_t port) {
//     fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
//     if (fd < 0) return false;

//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(port);
//     inet_pton(AF_INET, host, &addr.sin_addr);

//     int rc = ::connect(fd, (sockaddr*)&addr, sizeof(addr));
//     return (rc == 0 || errno == EINPROGRESS);
// }

// ssize_t MarketDataSocket::recv_data(void* buf, size_t len) {
//     return recv(fd, buf, len, 0);
// }

// int MarketDataSocket::get_fd() const {
//     return fd;
// }



// int MarketDataSocket::run() {
//     uint8_t buffer[8192];

//     while (running_) {
//         ssize_t n = recv(fd_, buffer, sizeof(buffer), 0);

//         if (n > 0) {
//             parser_.feed(buffer, n);
//         } else if (n == 0) {
//             // Exchange disconnected
//             return -1;
//         } else {
//             if (errno == EAGAIN || errno == EWOULDBLOCK)
//                 continue;
//             return -1;
//         }
//     }
//     return 0;
// }

#include "market_data_socket.hpp"

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <cerrno>
#include <vector>
#include <cstring>

bool MarketDataSocket::connect_to(const char* host, uint16_t port) {
    fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd_ < 0) {
        return false;
    }

    // Disable Nagle (low latency)
    int flag = 1;
    setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    // Increase recv buffer
    int rcvbuf = 4 * 1024 * 1024;
    setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        close();
        return false;
    }

    int rc = ::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (rc == 0) {
        return true; // Connected immediately
    }

    if (rc < 0 && errno == EINPROGRESS) {
        return true; // Non-blocking connect in progress
    }

    close();
    return false;
}

bool MarketDataSocket::send_subscription(const std::vector<uint16_t>& symbols) {
    std::vector<uint8_t> buf;
    buf.reserve(1 + 2 + symbols.size() * 2);

    buf.push_back(0xFF);  // subscribe command

    uint16_t count = htons(symbols.size());
    buf.insert(buf.end(),
               reinterpret_cast<uint8_t*>(&count),
               reinterpret_cast<uint8_t*>(&count) + sizeof(count));

    for (uint16_t s : symbols) {
        uint16_t sid = htons(s);
        buf.insert(buf.end(),
                   reinterpret_cast<uint8_t*>(&sid),
                   reinterpret_cast<uint8_t*>(&sid) + sizeof(sid));
    }

    ssize_t sent = ::send(fd_, buf.data(), buf.size(), 0);
    return sent == static_cast<ssize_t>(buf.size());
}


ssize_t MarketDataSocket::recv_data(void* buf, size_t len) {
    if (fd_ < 0) return -1;
    return ::recv(fd_, buf, len, 0);
}

ssize_t MarketDataSocket::send_data(const void* buf, size_t len) {
    if (fd_ < 0) return -1;
    return ::send(fd_, buf, len, MSG_NOSIGNAL);
}

int MarketDataSocket::get_fd() const {
    return fd_;
}

void MarketDataSocket::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

MarketDataSocket::~MarketDataSocket() {
    close();
}
