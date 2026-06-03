#include "nanomatch/matcher.hpp"
#include <memory>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>

namespace nanomatch {

void LatencyStats::record(std::uint64_t cycles) {
    ++count;
    sum_cycles += cycles;
    min_cycles = std::min(min_cycles, cycles);
    max_cycles = std::max(max_cycles, cycles);
    samples.push_back(cycles);
}

void LatencyStats::print_summary(const char* label) const {
    if (count == 0) {
        std::printf("%s: no samples\n", label);
        return;
    }
    auto pct = [&](double p) -> std::uint64_t {
        if (samples.empty()) {
            return 0;
        }
        std::vector<std::uint64_t> sorted = samples;
        std::sort(sorted.begin(), sorted.end());
        const std::size_t idx =
            static_cast<std::size_t>(p * static_cast<double>(sorted.size() - 1));
        return sorted[idx];
    };

    const double avg = static_cast<double>(sum_cycles) / static_cast<double>(count);
    std::printf("\n=== %s ===\n", label);
    std::printf("  samples : %llu\n", static_cast<unsigned long long>(count));
    std::printf("  avg     : %.0f cycles\n", avg);
    std::printf("  min     : %llu cycles\n", static_cast<unsigned long long>(min_cycles));
    std::printf("  max     : %llu cycles\n", static_cast<unsigned long long>(max_cycles));
    std::printf("  p50     : %llu cycles\n", static_cast<unsigned long long>(pct(0.50)));
    std::printf("  p90     : %llu cycles\n", static_cast<unsigned long long>(pct(0.90)));
    std::printf("  p99     : %llu cycles\n", static_cast<unsigned long long>(pct(0.99)));
}

void MatcherEngine::drain_ring(SpscRingBuffer<Trade, kTradeRingSize>& ring,
                               std::atomic<bool>& done,
                               std::atomic<std::size_t>& logged) {
    while (!done.load(std::memory_order_acquire) || ring.size_approx() > 0) {
        if (auto trade = ring.try_pop()) {
            logged.fetch_add(1, std::memory_order_relaxed);
            (void)trade;
        } else {
            std::this_thread::yield();
        }
    }
}

ReplayResult MatcherEngine::replay_csv(
    const std::string& path,
    bool use_async_logger)
{
    

    const auto events = load_csv(path);

    

    OrderBook book;
    auto ring =
    std::make_unique<SpscRingBuffer<Trade, kTradeRingSize>>();

    std::atomic<bool> logger_done{false};
    std::atomic<std::size_t> async_logged{0};
    std::thread logger_thread;

    if (use_async_logger) {
        logger_thread = std::thread(
    [&]()
    {
        drain_ring(*ring, logger_done, async_logged);
    });
    }

    ReplayResult result;
    std::size_t trade_cursor = 0;
    for (const auto& ev : events) {
        const std::uint64_t t0 = rdtsc();
        bool ok = true;
        switch (ev.type) {
            case OrderType::Limit:
                ok = book.add_limit(ev.id, ev.side, ev.price, ev.qty, ev.ts);
                break;
            case OrderType::Market:
                ok = book.add_market(ev.id, ev.side, ev.qty, ev.ts);
                break;
            case OrderType::Cancel:
                ok = book.cancel(ev.id);
                break;
        }
        const std::uint64_t t1 = rdtsc();
        if (ok) {
            result.latency.record(t1 - t0);
            ++result.processed;
        }

        if (use_async_logger) {
            const auto& all_trades = book.trades();
            for (std::size_t i = trade_cursor; i < all_trades.size(); ++i) {
                if (!ring->try_push(all_trades[i])) {
                    break;
                }
            }
            trade_cursor = all_trades.size();
        }
    }

    if (use_async_logger) {
        logger_done.store(true, std::memory_order_release);
        if (logger_thread.joinable()) {
            logger_thread.join();
        }
        result.async_logged = async_logged.load();
    } else {
        result.trades = book.trade_count();
    }

    result.trades = book.trade_count();
    return result;
}

}  // namespace nanomatch
