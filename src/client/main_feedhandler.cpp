#include "feed_handler.hpp"
#include "visualizer.hpp"

#include <thread>
#include <iostream>

int main() {
    try {
        FeedHandler handler("127.0.0.1", 9876);

        Visualizer viz(handler);
        std::thread ui([&]() { viz.run(); });

        handler.run();

        ui.join();
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }
    return 0;
}
