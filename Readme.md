#   Market Data Exchange Simulator
A high-performance, low-latency market data exchange simulator written in **Modern C++**. This system simulates tick-by-tick market data for multiple symbols and allows multiple clients to connect concurrently to receive historical and live data.

This project demonstrates core system design concepts used in real-world trading infrastructure, such as **electronic exchanges, feed handlers, and market data gateways.**

---
#  System Architecture
The system follows a decoupled architecture where the Exchange serves as the "source of truth" and the Feed Handler acts as the consumer.

Key Design Highlights
Non-blocking I/O: Powered by epoll for efficient handling of multiple concurrent client connections.

**Stochastic Modeling:** Each symbol maintains an independent price process.

**Shared Protocol:** Low-latency communication via a header-only common module.

**Scalability:** Optimized for high-frequency tick generation and low-latency distribution.

**Memory Safety:** Built with Address Sanitizer (ASAN) and Undefined Behavior Sanitizer (UBSAN).

---
<pre>
 ├── CMakeLists.txt              # Root CMake configuration
 ├── README.md                   # Project documentation
 └── src/
    ├── CMakeLists.txt          # Registers all submodules
    ├── common/                 # Shared protocol & definitions
    │   └── protocol.hpp
    ├── server/                 # Exchange simulator (The Provider)
    │   ├── main.cpp
    │   ├── exchange_simulator.cpp
    │   ├── exchange_simulator.hpp
    │   ├── ConfigManager.cpp
    │   └── ConfigManager.hpp
    └── client                 Feed handler (The Consumer)
        ├── main_feedhandler.cpp
        ├── feed_handler.cpp
        ├── parser.cpp
        ├── market_data_socket.cpp
        └── visualizer.cpp
</pre>

### Folder Responsibilities

| Directory | Description |
|----------|-------------|
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

```
git clone <repository-url>
cd MarketDataSystem

cmake -S . -B build
cmake --build build -j
```


```
    build/
    ├── exchange_simulator
    └── feed_handler
```

## 🧾 Configuration File (Exchange Simulator)

The exchange simulator uses a configuration file to define runtime parameters such as:
```
    Symbols traded
    Initial price range
    Volatility per symbol
    Tick generation interval
    Session duration
    Network port
```
<!-- // <\pre> -->

## ▶️ Running the Exchange Simulator
Basic Usage
```
./exchange_simulator config.ini RUN_MODE 
```
Supported Command-Line Arguments

| Argument |	Description |
|----------|-------------|
| `--config <file>`  |	Path to configuration file (required) |
<!-- | `--port <port>	`| Override port from config | -->
<!-- | `--symbols <N>	`| Override number of symbols | -->
<!-- |` --log-level <level>` |	Logging verbosity | -->

## 📡 Running the Client (Feed Handler)

The client connects to the exchange and subscribes to one or more symbols.

Example Usage
```
./feed_handler 
```

Client Arguments
```
NOT IMPLEMENTED
```

## 🔄 System Data Flow
### Exchange Simulator

```
    Generates independent stochastic price processes
    Produces bid/ask quotes and trades
    Handles multiple clients using epoll
```

### Feed Handler
```text
    Connects to exchange
    Requests historical market data
    Subscribes to live data stream
    Parses and processes incoming messages
```
## 🧠 Design Highlights

```
    Epoll-based non-blocking I/O
    Independent price process per symbol
    Shared protocol via header-only common module
    Low-latency, scalable architecture
    Sanitizer-enabled builds (ASAN + UBSAN)
```

---

## ▶️ Running the Simulation Script

To simplify running the exchange simulator along with multiple feed handlers, a helper script is provided.

This script:
- Launches the exchange simulator
- Starts multiple feed handlers
- Pins each process to specific CPU cores
- Redirects logs to files
- Ensures clean shutdown on `Ctrl+C`

### Script Location
```
    scripts/run_simulation.sh
```

### Make the script executable (one-time step):

```bash
    chmod +x scripts/run_simulation.sh
```

## What it does

* Starts the exchange simulator on a dedicated CPU core.
* Launches multiple feed handlers, each pinned to a separate CPU.
* Redirects logs to scripts/logs/.
* Stops all processes cleanly on Ctrl+C.

```bash
    CPU Affinity
    SIM_CPU=2
    CLIENT_CPUS=(3 4 5 6)
```

Adjust CPU IDs based on your system.
```
Logs
scripts/logs/
 ├── simulator.log
 ├── feedhandler_cpu3.log
 ├── feedhandler_cpu4.log
 ├── feedhandler_cpu5.log
 └── feedhandler_cpu6.log
```
### Run
```bash 
    scripts/run_simulation.sh
```

