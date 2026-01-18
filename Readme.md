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
<!-- <pre> -->
```ini
{
    ; ================================
; Exchange Simulator Configuration
; ================================
[SERVER]
; Number of threads used for non-IO tasks
THREADS = 4
SERVER_IP_ADD = 127.0.0.1
PORT = 9876

; ----------------
; Exchange settings
; ----------------
[EXCHANGE]
; Total number of symbols to simulate
SYMBOLS = 100

; Initial price range per symbol (₹)
PRICEMIN = 100.0
PRICEMAX = 5000.0

; ----------------
; Market dynamics
; ----------------
[MARKET]

; Drift (μ)
;  0.0   -> neutral market
;  0.05  -> bull market
; -0.05  -> bear market
DRIFT = 0.0

; Volatility range (σ)
VOLATILITYMIN = 0.01
VOLATILITYMAX = 0.06

; Bid-ask spread range (percentage of price)
; 0.0005 = 0.05%
; 0.0020 = 0.20%
SPREADMIN = 0.0005
SPREADMAX = 0.0020

;----------------
; Tick generation
; ----------------
[TICKS] 
; Tick rate range (messages per second)
; Used to randomly or deterministically resolve runtime rate
RATEMIN = 10000
RATEMAX = 500000
TICKSRATE=300000

; Time delta (dt) in seconds
; 0.001 = 1 ms
dT = 0.00001
m_runDurationSec = 10

; ----------------
; Message distribution
; ----------------
[messages]
; Quote vs Trade distribution
; Must sum to 1.0
quote_ratio = 0.70
trade_ratio = 0.30

; ----------------
; Run mode configuration
; ----------------
[MODE]
; Default run mode
; R = Random generation
; M = Manual (load symbol parameters from file)
DEFAULT = R

; Manual symbol configuration file (used only if mode = M)
MANUALFILE = ../configs/ManualSymbols.ini

; ----------------
; Logging (optional, future-proof)
; ----------------
[LOGGING]
; Levels: TRACE, DEBUG, INFO, WARN, ERROR
LEVEL = INFO
}
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