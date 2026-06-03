#include "nanomatch/matcher.hpp"
#include "nanomatch/order_book.hpp"
#include "nanomatch/order_book_stl.hpp"
#include "nanomatch/timing.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void print_usage() {
    std::printf("NANOMATCH - Ultra-Low Latency Order Matching Engine\n\n");
    std::printf("Usage:\n");
    std::printf("  nanomatch_cli replay <orders.csv> [--no-async] [--latency-out <file.csv>]\n");
    std::printf("  nanomatch_cli demo\n");
    std::printf("  nanomatch_cli compare <orders.csv>\n");
}

bool write_latency_csv(const std::string& path, const nanomatch::LatencyStats& stats) {
    std::ofstream out(path);
    if (!out) {
        return false;
    }
    out << "event_index,cycles\n";
    for (std::size_t i = 0; i < stats.samples.size(); ++i) {
        out << i << "," << stats.samples[i] << "\n";
    }
    return true;
}

void run_demo() {
    nanomatch::OrderBook book;

    book.add_limit(1, nanomatch::Side::Buy, 100000, 100, 1);
    book.add_limit(2, nanomatch::Side::Sell, 100500, 50, 2);
    book.add_limit(3, nanomatch::Side::Buy, 100500, 30, 3);  // crosses — partial fill

    std::printf("Best bid: %lld\n", static_cast<long long>(book.best_bid()));
    std::printf("Best ask: %lld\n", static_cast<long long>(book.best_ask()));
    std::printf("Trades: %zu\n", book.trades().size());
    for (const auto& t : book.trades()) {
        std::printf("  maker=%llu taker=%llu px=%lld qty=%u\n",
                    static_cast<unsigned long long>(t.maker_id),
                    static_cast<unsigned long long>(t.taker_id),
                    static_cast<long long>(t.price), t.qty);
    }

    book.clear_trades();
    book.add_market(4, nanomatch::Side::Sell, 40, 4);
    std::printf("After market sell, trades=%zu active_orders=%zu\n", book.trades().size(),
                book.active_orders());
}

void run_compare(const std::string& path) {
    const auto events = nanomatch::load_csv(path);

    nanomatch::OrderBook fast;
    nanomatch::OrderBookStl baseline;

    const auto run_book = [&](auto& book) {
        std::uint64_t cycles = 0;
        for (const auto& ev : events) {
            const std::uint64_t t0 = nanomatch::rdtsc();
            switch (ev.type) {
                case nanomatch::OrderType::Limit:
                    book.add_limit(ev.id, ev.side, ev.price, ev.qty, ev.ts);
                    break;
                case nanomatch::OrderType::Market:
                    book.add_market(ev.id, ev.side, ev.qty, ev.ts);
                    break;
                case nanomatch::OrderType::Cancel:
                    book.cancel(ev.id);
                    break;
            }
            cycles += nanomatch::rdtsc() - t0;
        }
        return cycles;
    };

    const std::uint64_t fast_cycles = run_book(fast);
    const std::uint64_t stl_cycles = run_book(baseline);

    std::printf("Events: %zu\n", events.size());
    std::printf("Optimized total cycles: %llu\n", static_cast<unsigned long long>(fast_cycles));
    std::printf("STL baseline cycles:    %llu\n", static_cast<unsigned long long>(stl_cycles));
    if (fast_cycles > 0) {
        const double speedup = static_cast<double>(stl_cycles) / static_cast<double>(fast_cycles);
        std::printf("Speedup: %.2fx\n", speedup);
    }
    std::printf("Optimized trades: %zu | STL trades: %zu\n", fast.trades().size(),
                baseline.trades().size());
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const std::string cmd = argv[1];
    if (cmd == "demo") {
        run_demo();
        return 0;
    }

    if (cmd == "replay" && argc >= 3) {
        
        bool async_log = true;
        std::string latency_out;
        for (int i = 3; i < argc; ++i) {
            if (std::strcmp(argv[i], "--no-async") == 0) {
                async_log = false;
            }
            if (std::strcmp(argv[i], "--latency-out") == 0 && i + 1 < argc) {
                latency_out = argv[++i];
            }
        }
         
        nanomatch::MatcherEngine engine;
        

        const auto result = engine.replay_csv(argv[2], async_log);
        
        std::printf("Processed: %zu | Trades in book: %zu | Async logged: %zu\n",
                    result.processed, result.trades, result.async_logged);
        result.latency.print_summary("Per-event latency (RDTSC cycles)");
        if (!latency_out.empty()) {
            if (write_latency_csv(latency_out, result.latency)) {
                std::printf("Saved latency samples to: %s\n", latency_out.c_str());
            } else {
                std::fprintf(stderr, "Failed to write latency CSV: %s\n", latency_out.c_str());
                return 1;
            }
        }
        return 0;
    }

    if (cmd == "compare" && argc >= 3) {
        run_compare(argv[2]);
        return 0;
    }

    print_usage();
    return 1;
}
