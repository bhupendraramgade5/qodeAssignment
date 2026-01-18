# Market Data Exchange Simulator

A low-latency market data exchange simulator written in modern C++.
The system simulates tick-by-tick market data for multiple symbols and allows multiple clients to connect concurrently to receive historical and live data.

This project demonstrates system design concepts used in real-world trading infrastructure such as exchanges, feed handlers, and market data gateways.

---

## 📁 Project Folder Structure

CMakeLists.txt # Root CMake configuration
README.md # Project documentation
src/
├── CMakeLists.txt # Registers all submodules
├── common/ # Shared protocol & definitions
│ ├── CMakeLists.txt
│ └── protocol.hpp
├── server/ # Exchange simulator
│ ├── CMakeLists.txt
│ ├── main.cpp
│ ├── exchange_simulator.cpp
│ ├── exchange_simulator.hpp
│ ├── ConfigManager.cpp
│ └── ConfigManager.hpp
└── client/ # Feed handler / consumer
├── CMakeLists.txt
├── main_feedhandler.cpp
├── feed_handler.cpp
├── parser.cpp
├── market_data_socket.cpp
└── visualizer.cpp


### Folder Responsibilities

| Directory | Description |
|---------|-------------|
| `common/` | Shared protocol definitions used by server and client |
| `server/` | Exchange simulator that generates market data |
| `client/` | Feed handler that consumes and processes data |

---

## ⚙️ Build Instructions

### Prerequisites

- Linux / WSL
- CMake ≥ 3.20
- GCC or Clang (C++17)
- Internet access (Boost is fetched automatically)

### Build Steps

```bash
git clone <repository-url>
cd MarketDataSystem

cmake -S . -B build
cmake --build build -j
Output Binaries
build/
 ├── src/server/exchange_simulator
 └── src/client/feed_handler
🧾 Configuration File (Exchange Simulator)
The exchange simulator uses a configuration file to define runtime parameters such as:

Symbols traded

Initial price range

Volatility per symbol

Tick generation interval

Session duration

Network port

Example Configuration File (config.json)
{
  "symbols": ["AAPL", "GOOG", "MSFT", "TSLA"],
  "initial_price_range": {
    "min": 100,
    "max": 5000
  },
  "volatility_range": {
    "min": 0.01,
    "max": 0.06
  },
  "tick_interval_ms": 10,
  "session_duration_sec": 3600,
  "listen_port": 9000
}
The configuration is parsed and validated by ConfigManager.

▶️ Running the Exchange Simulator
Basic Usage
./exchange_simulator --config config.json
Supported Command-Line Arguments
Argument	Description
--config <file>	Path to configuration file (required)
--port <port>	Override port from config
--symbols <N>	Override number of symbols
--log-level <level>	Logging verbosity
Example
./exchange_simulator \
  --config config.json \
  --port 9100 \
  --log-level DEBUG
📡 Running the Client (Feed Handler)
The client connects to the exchange and subscribes to one or more symbols.

Example Usage
./feed_handler \
  --host 127.0.0.1 \
  --port 9000 \
  --symbols AAPL,MSFT,TSLA
Client Arguments
Argument	Description
--host	Exchange IP address
--port	Exchange port
--symbols	Comma-separated list of symbols
--from-start	Receive historical data from session start
--live	Subscribe to live tick-by-tick data
🔄 System Data Flow
Exchange Simulator

Generates independent stochastic price processes

Produces bid/ask quotes and trades

Handles multiple clients using epoll

Feed Handler

Connects to exchange

Requests historical market data

Subscribes to live data stream

Parses and processes incoming messages

🧠 Design Highlights
Epoll-based non-blocking I/O

Independent price process per symbol

Shared protocol via header-only common module

Low-latency, scalable architecture

Sanitizer-enabled builds (ASAN + UBSAN)

🧪 Sanitizers
Sanitizers are enabled by default.

To disable them:

cmake -S . -B build -DBUILD_SANITIZERS=OFF
📌 Notes
All CMake targets have globally unique names

common is an INTERFACE (header-only) library

Designed to scale to more symbols and clients

🚀 Possible Extensions
Full order book simulation

Multicast / UDP market data

Binary wire protocol

Market replay from recorded data

Strategy backtesting engine

👤 Author
Bhupendra Ramgade
Software Engineer – Systems / C++


---

If you want next, I can:
- Tighten this for **interview submission**
- Add **architecture diagrams**
- Add **example logs**
- Add **performance/latency metrics section**

Just say 👍