#ifndef VIS_H
#define VIS_H
#include <atomic>
#include <chrono>
#include"feed_handler.hpp"

// class Visualizer {
// public:
//     Visualizer();
//     void run();

// private:
//     std::chrono::steady_clock::time_point start_time;
// };

class Visualizer {
public:
    // Visualizer();
    explicit Visualizer(const FeedHandler& fh);
    void run();
private:
    const FeedHandler& feed_handler_;
    std::chrono::steady_clock::time_point start_time;
};

#endif