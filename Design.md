# Market Data Exchange Simulator & Feed Handler – System Design

This document describes the architecture, design decisions, and performance considerations of the **Market Data Exchange Simulator** and **Feed Handler** system. The goal of this project is to simulate a realistic low-latency market data pipeline similar to those used in real exchanges and trading infrastructure.

---

## 1. System Architecture

### 1.a Client–Server Architecture

```
+----------------------+        TCP (Binary Protocol)        +----------------------+
|  Exchange Simulator  |  --------------------------------> |     Feed Handler     |
|  (Server)            |                                     |  (Client)            |
|                      | <--------------------------------  |                      |
|  - Tick Generator    |        Subscription Messages        |  - Parser            |
|  - Symbol Engine     |                                     |  - Symbol Cache      |
|  - Broadcast Engine  |                                     |  - Visualizer        |
+----------------------+                                     +----------------------+
```

* The **Exchange Simulator** acts as a TCP server producing market data.
* Multiple **Feed Handler** clients connect, subscribe to symbols, and receive filtered data.

---

### 1.b Thread Model

#### Server (Exchange Simulator)

* **Single-threaded, event-driven** design using `epoll` (edge-triggered)
* Logical responsibilities within the same thread:

  * Accept new client connections
  * Handle subscription messages
  * Generate ticks at configured rate
  * Broadcast messages to subscribed clients

This mirrors real low-latency systems where avoiding cross-thread synchronization is critical.

#### Client (Feed Handler)

* **Network thread**: epoll-based receive loop, parsing, cache updates
* **UI / Visualizer thread**: periodically reads symbol cache and renders terminal output

This split ensures visualization does not block the network hot path.

---

### 1.c Data Flow

```
[Tick Generator]
      ↓
[Symbol Price Evolution (GBM)]
      ↓
[MarketMessage Construction]
      ↓
[Binary Encoding + Endianness]
      ↓
[Broadcast to Clients]
      ↓
[TCP Receive Buffer]
      ↓
[StreamBuffer]
      ↓
[Parser]
      ↓
[Lock-free Symbol Cache]
      ↓
[Visualizer / Statistics]
```

---

## 2. Geometric Brownian Motion (GBM)

### 2.a Mathematical Formulation

Each symbol price evolves according to:

```
S(t+dt) = S(t) * exp((μ - 0.5σ²)dt + σ√dt * Z)
```

Where:

* `μ` = drift
* `σ` = volatility
* `dt` = time step
* `Z` = standard normal random variable

---

### 2.b Implementation Approach

* A **normal distribution** is generated using `std::normal_distribution`
* Internally, this uses a Box–Muller–like transformation
* Each symbol owns its own RNG (`std::mt19937_64`) to avoid correlation

---

### 2.c Parameter Choices

* `μ` (market drift): small (≈ 0) to avoid runaway prices
* `σ`: randomized per symbol within configured range
* `dt`: derived from tick rate (`1 / ticks_per_second`)

---

### 2.d Realism Safeguards

* Prices clamped to a minimum threshold
* Bid/ask spread enforced symmetrically
* Safety checks to prevent crossed markets

---

## 3. Network Layer Design

### 3.a Server Networking

* TCP sockets in non-blocking mode
* `epoll` with `EPOLLET` for scalability
* One epoll instance handles:

  * Listening socket
  * All connected clients

---

### 3.b Client Networking

* Single epoll loop for receive-side only
* Subscription sent once after successful connect
* All parsing and cache updates happen on the same thread

---

### 3.c Buffer Management

#### StreamBuffer

* Byte-oriented buffer for TCP reassembly
* Supports:

  * Append
  * Consume
  * Partial message handling
* Compacts only when offset exceeds threshold to reduce copies

---

### 3.d Reconnection Logic (Design Intent)

* Detect disconnect (`recv == 0` or fatal error)
* Exponential backoff reconnect attempts
* Re-send subscription upon reconnect

> **Note:** Basic reconnect detection exists; full backoff logic is a planned enhancement.

---

### 3.e Flow Control Strategy

* Server checks subscription set before sending
* Slow or broken clients are disconnected
* Avoids blocking send path

This prioritizes **system liveness** over fairness.

---

## 4. Memory Management Strategy

### 4.a Buffer Lifecycle

* Network buffers are stack-allocated in hot path
* `StreamBuffer` owns a reusable heap buffer

---

### 4.b Allocation Patterns

* No dynamic allocation in tick generation loop
* No per-message heap allocation on client side

---

### 4.c Alignment & Cache Considerations

* `MarketMessage` is packed for wire efficiency
* Symbol cache is array-indexed by `symbol_id`
* Sequential memory layout improves cache locality

---

### 4.d Pool Usage (Design Intent)

* Server could evolve to use object pools for messages
* Client avoids pools since it stores only latest state

---

## 5. Concurrency Model

### 5.a Lock-Free Techniques

* Single-writer, multiple-reader symbol cache
* Versioned state publication

---

### 5.b Memory Ordering

* `memory_order_release` on writes
* `memory_order_acquire` on reads
* Ensures visibility without mutexes

---

### 5.c Single-Writer / Multi-Reader Pattern

```
Writer:
  version++ (odd)
  write data
  version++ (even)

Reader:
  read v1
  copy data
  read v2
  accept if v1 == v2 and even
```

This guarantees consistency with minimal overhead.

---

## 6. Visualization Design

### 6.a Update Strategy

* Separate visualization thread
* Periodic polling of symbol cache
* Never blocks network thread

---

### 6.b ANSI Escape Codes

* Clear screen
* Cursor repositioning
* Minimal redraw to reduce flicker

---

### 6.c Non-Blocking Statistics

* Atomics for message count and gaps
* Read-only access from UI thread

---

## 7. Performance Optimization

### 7.a Hot Path Identification

Hot path includes:

* Tick generation
* Binary encoding
* `send()` on server
* `recv()` → parse → cache update on client

---

### 7.b Cache Optimization

* Array-based symbol cache
* No pointer chasing
* Sequential memory access patterns

---

### 7.c False Sharing Prevention (Planned)

* Cache-line padding for symbol states
* Separate frequently-updated counters

---

### 7.d System Call Minimization

* Edge-triggered epoll
* Batch reads in receive loop
* No blocking syscalls in hot path

---

## 8. Config Manager Design

### Purpose

The **ConfigManager** centralizes all runtime parameters:

* Network settings (IP, port)
* Tick rate
* Volatility / spread ranges
* Symbol count
* Run duration

---

### Design Intent

* Single config file parsed at startup
* Shared schema usable by:

  * Exchange Simulator
  * Feed Handler (future extension)

---

### Why This Matters

* Enables reproducible simulations
* Allows identical configs for server and client
* Mirrors real trading systems where feed handlers and exchanges share config schemas

---

## Final Notes

This system is intentionally designed as a **low-latency prototype**, prioritizing:

* Determinism
* Correctness of data flow
* Clear separation of concerns

While not fully production-hardened, the architecture closely resembles real market data infrastructure used in trading systems.
