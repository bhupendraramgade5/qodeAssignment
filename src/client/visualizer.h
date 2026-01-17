#pragma once
#include <atomic>
#include <chrono>

class Visualizer {
public:
    Visualizer();
    void run();

private:
    std::chrono::steady_clock::time_point start_time;
};
