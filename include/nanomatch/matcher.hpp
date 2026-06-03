#pragma once

#include "nanomatch/csv_reader.hpp"
#include "nanomatch/order_book.hpp"
#include "nanomatch/spsc_ring.hpp"
#include "nanomatch/timing.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace nanomatch {

struct LatencyStats {
    std::uint64_t count = 0;
    std::uint64_t sum_cycles = 0;
    std::uint64_t min_cycles = UINT64_MAX;
    std::uint64_t max_cycles = 0;
    std::vector<std::uint64_t> samples;

    void record(std::uint64_t cycles);
    void print_summary(const char* label) const;
};

struct ReplayResult {
    std::size_t processed = 0;
    std::size_t trades = 0;
    std::size_t async_logged = 0;
    LatencyStats latency;
};

// Replays CSV events through OrderBook; optional async SPSC trade logger thread.
class MatcherEngine {
public:
    static constexpr std::size_t kTradeRingSize = 65536;

    ReplayResult replay_csv(const std::string& path, bool use_async_logger = true);

private:
    void drain_ring(SpscRingBuffer<Trade, kTradeRingSize>& ring,
                    std::atomic<bool>& done,
                    std::atomic<std::size_t>& logged);
};

}  // namespace nanomatch
