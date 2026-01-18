#include "visualizer.hpp"

#include <iostream>
#include <thread>
#include <iomanip>

// extern std::atomic<uint64_t> g_messages;
// extern std::atomic<uint64_t> g_seq_gaps;


Visualizer::Visualizer(const FeedHandler& fh)
    : feed_handler_(fh),
      start_time(std::chrono::steady_clock::now()) {}

void Visualizer::run() {
    uint64_t last_count = 0;

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        uint64_t total = feed_handler_.message_count();
        uint64_t rate = (total - last_count) * 2; // 500ms window
        last_count = total;

        auto uptime =
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start_time).count();

        // Clear screen & move cursor home
        std::cout << "\033[2J\033[H";

        std::cout << "=== NSE Market Data Feed Handler ===\n";
        std::cout << "Connected to: localhost:9876\n";
        std::cout << "Uptime: " << uptime << " sec\n\n";

        std::cout << "Messages Processed: " << total << "\n";
        std::cout << "Receive Rate:       " << rate << " msg/sec\n";
        std::cout << "Sequence Gaps:      " << feed_handler_.sequence_gaps() << "\n";

        std::cout << "\nPress Ctrl+C to exit\n";
        std::cout.flush();
    }
}
