# NANOMATCH
## A Cache-Conscious Low-Latency Limit Order Matching Engine in Modern C++

> A systems-engineering project built to explore how modern exchanges process orders with predictable latency through careful memory management, cache-aware design, lock-free communication, and hardware-conscious data structures.

---

# Overview

NANOMATCH is a miniature stock exchange matching engine designed to simulate the core component of an electronic exchange: the **Limit Order Book (LOB)**.

Unlike most trading simulations that focus on strategies and finance, this project focuses on the systems side of the problem:

- How orders are stored in memory
- How matching is performed efficiently
- How cache misses affect latency
- Why memory allocation matters
- How lock-free queues reduce contention
- How low-latency systems are benchmarked

The project includes:

- A fully functional matching engine
- Price-time priority matching
- Market and limit orders
- Order cancellation
- Custom memory pool allocator
- Cache-conscious order book design
- Optional lock-free asynchronous logging
- Benchmarking infrastructure
- Latency measurement
- STL baseline comparison

---

# Why This Project Exists

Modern exchanges process millions of events every second.

At that scale, the bottleneck is often not the matching algorithm itself.

The bottleneck becomes:

- Cache misses
- Dynamic memory allocation
- Synchronization overhead
- Data movement
- Tail latency spikes

NANOMATCH was built to explore these challenges in a controlled environment while learning modern C++ systems programming.

---

# What Is A Matching Engine?

A matching engine receives:

- Buy Orders
- Sell Orders
- Cancel Requests

and maintains a continuously updated Limit Order Book.

When a buyer is willing to pay a seller's price, a trade occurs.

The matching engine determines:

1. Who gets matched
2. At what price
3. In what order

---

# Price-Time Priority

Rule 1: Better Price Wins

- Higher buy price has priority.
- Lower sell price has priority.

Rule 2: Earlier Orders Win

When two orders have the same price:

- Older order executes first.
- Newer order waits.

This ensures fairness.

---

# High-Level Architecture

CSV / Market Data
        |
        v
CSV Parser
        |
        v
Matcher Engine
        |
        +-------------------+
        |                   |
        v                   v
Order Book          Latency Recorder
        |
        v
Trade Generation
        |
        v
Optional Async Logger

---

# Systems Concepts Behind This Project

## CPU Cache

Modern CPUs are dramatically faster than RAM.

To reduce memory access time, CPUs use:

- L1 Cache
- L2 Cache
- L3 Cache

The closer the data is to the CPU, the faster it can be accessed.

## Cache Locality

Data stored close together in memory is likely to be loaded together.

Example:

std::vector<Order>

is generally cache friendly because elements are contiguous.

A tree-based structure such as std::map is often less cache friendly because nodes are scattered throughout memory.

## Cache Misses

A cache miss occurs when requested data is not present in cache and must be fetched from a slower memory level.

Reducing cache misses is one of the primary goals of the optimized order book.

---

# Memory Management Strategy

## Why Not Use new/delete For Every Order?

Dynamic allocation introduces:

- Allocator overhead
- Fragmentation
- Unpredictable latency

These effects become visible when processing large numbers of orders.

## Object Pool

Instead of allocating memory repeatedly, NANOMATCH preallocates storage.

Benefits:

- Predictable memory usage
- Better cache locality
- Reduced allocation overhead
- Stable latency

---

# Parsing Strategy

Version 1 uses CSV files for simplicity.

Example:

LIMIT,1,BUY,100000,50,1

Each row is converted into a MarketEvent object.

## Why CSV?

Real exchanges use binary protocols such as:

- ITCH
- FIX
- OUCh

However CSV is:

- Human readable
- Easy to debug
- Simple to test

Future versions may support binary feed decoding.

---

# mmap Support

On Linux, large files can be accessed using mmap().

Instead of repeatedly reading data from disk into buffers, the operating system maps the file into memory.

Benefits:

- Fewer memory copies
- Better large-file replay performance
- Reduced I/O overhead

---

# Concurrency Model

The matching engine may optionally push generated trades into a lock-free SPSC ring buffer.

Matcher Thread
      |
      v
SPSC Ring Buffer
      |
      v
Logger Thread

This allows trade logging without blocking the matching path.

---

# Project Structure

nanomatch/
├── include/nanomatch/
├── src/
├── bench/
├── tests/
├── tools/
├── data/
├── README.md
├── INSTALL.md
└── STUDY_GUIDE.md

---

# File-by-File Architecture

## include/nanomatch/types.hpp

Defines fundamental types used across the project.

Contains:

- OrderId
- Price
- Quantity
- Timestamp
- Side enum
- OrderType enum

Purpose:

Provide consistent data representation throughout the engine.

---

## include/nanomatch/order.hpp

Defines:

### OrderNode

Represents one active order.

Stores:

- ID
- Price
- Quantity
- Side
- Timestamp
- Queue linkage information

### Trade

Represents one executed trade.

---

## include/nanomatch/memory_pool.hpp

Implements:

ObjectPool<T>

Responsibilities:

- Preallocate storage
- Allocate slots
- Recycle slots
- Avoid frequent heap allocation

Key Methods:

- allocate()
- deallocate()
- get()
- capacity()
- size_active()

---

## include/nanomatch/spsc_ring.hpp

Implements:

Single Producer Single Consumer Ring Buffer

Responsibilities:

- Lock-free communication
- Trade transfer
- Reduced synchronization cost

Key Methods:

- try_push()
- try_pop()
- size_approx()

---

## include/nanomatch/timing.hpp

Provides:

rdtsc()

Used for:

- Low-overhead latency measurement
- Cycle-level benchmarking

---

## include/nanomatch/order_book.hpp

Core optimized order book.

### PriceLevel

Stores:

- Price
- Total Quantity
- Head/Tail queue references

### OrderBook

Main matching structure.

Public Methods:

- add_limit()
- add_market()
- cancel()
- best_bid()
- best_ask()
- bid_depth()
- ask_depth()
- trade_count()
- trades()
- clear_trades()
- active_orders()

Private Helpers:

- match_incoming()
- match_against_level()
- append_to_level()
- remove_from_level()
- find_or_create_level()
- level_for()
- remove_empty_level()

---

## include/nanomatch/order_book_stl.hpp

Reference implementation.

Uses:

- std::map
- std::deque

Purpose:

Provide a baseline for comparison against the optimized implementation.

---

## include/nanomatch/csv_reader.hpp

Defines:

### MarketEvent

Represents one parsed row from input.

### load_csv()

Loads events and returns a vector of MarketEvent objects.

---

## include/nanomatch/matcher.hpp

Coordinates replay execution.

### LatencyStats

Responsible for:

- Recording latency samples
- Calculating p50
- Calculating p90
- Calculating p99

### ReplayResult

Stores replay summary.

### MatcherEngine

Main replay controller.

Methods:

- replay_csv()
- drain_ring()

---

# Source File Responsibilities

## src/order_book.cpp

Implements:

- Matching logic
- Price-time priority
- Queue management
- Cancellation
- Market order execution

This is the heart of the engine.

---

## src/order_book_stl.cpp

Implements the STL baseline.

Purpose:

Measure performance differences between standard containers and optimized structures.

---

## src/csv_reader.cpp

Implements:

- CSV parsing
- String-to-enum conversion
- Linux mmap fast path

Key Functions:

- parse_side()
- parse_type()
- parse_event_line()
- load_csv()
- load_csv_mmap()

---

## src/matcher.cpp

Implements:

- Replay execution
- Latency collection
- Async logging

Key Functions:

- LatencyStats::record()
- LatencyStats::print_summary()
- MatcherEngine::replay_csv()
- MatcherEngine::drain_ring()

---

## src/main.cpp

Application entry point.

Commands:

- demo
- replay
- compare

Responsible for CLI interaction.

---

# Benchmarking

Benchmarks compare:

1. STL implementation
2. Optimized implementation

Metrics:

- Throughput
- Average Latency
- p50
- p90
- p99

---

# Testing

tests/test_order_book.cpp

Validates:

- Order insertion
- Matching correctness
- Cancellation
- STL baseline behavior

---

# Current Features

- Limit Orders
- Market Orders
- Order Cancellation
- Price-Time Priority
- Object Pool Allocation
- Lock-Free SPSC Queue
- CSV Replay
- Linux mmap Support
- Benchmarking
- Latency Statistics

---

# Future Improvements

## Market Data

- NASDAQ ITCH parser
- PCAP replay
- Binary feed decoding

## Performance

- Branch prediction optimization
- False-sharing analysis
- NUMA awareness
- Cache profiling

## Scaling

- Multi-symbol support
- Symbol sharding
- Parallel processing

---

# Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

# Run

```bash
nanomatch_cli demo
nanomatch_cli replay data/sample_orders.csv
nanomatch_cli compare data/orders.csv
```

---

# Key Takeaway

The primary lesson of NANOMATCH is that performance is often determined by memory layout, cache behavior, allocation strategy, and synchronization costs rather than raw algorithmic complexity alone.

Understanding how data moves through a computer is often more valuable than simply writing correct code.
