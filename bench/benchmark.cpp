#include "nanomatch/order_book.hpp"
#include "nanomatch/order_book_stl.hpp"
#include "nanomatch/timing.hpp"

#include <benchmark/benchmark.h>
#include <random>

namespace {

constexpr nanomatch::Price kBasePrice = 1000000;

void fill_limit_orders(nanomatch::OrderBook& book, std::size_t n, std::mt19937& rng) {
    std::uniform_int_distribution<int> price_jitter(-500, 500);
    for (std::size_t i = 0; i < n; ++i) {
        const auto side = (i % 2 == 0) ? nanomatch::Side::Buy : nanomatch::Side::Sell;
        const nanomatch::Price px =
            kBasePrice + static_cast<nanomatch::Price>(price_jitter(rng));
        book.add_limit(static_cast<nanomatch::OrderId>(i + 1), side, px, 10,
                       static_cast<nanomatch::Timestamp>(i));
    }
}

void fill_limit_orders_stl(nanomatch::OrderBookStl& book, std::size_t n, std::mt19937& rng) {
    std::uniform_int_distribution<int> price_jitter(-500, 500);
    for (std::size_t i = 0; i < n; ++i) {
        const auto side = (i % 2 == 0) ? nanomatch::Side::Buy : nanomatch::Side::Sell;
        const nanomatch::Price px =
            kBasePrice + static_cast<nanomatch::Price>(price_jitter(rng));
        book.add_limit(static_cast<nanomatch::OrderId>(i + 1), side, px, 10,
                       static_cast<nanomatch::Timestamp>(i));
    }
}

}  // namespace

static void BM_Optimized_AddLimit(benchmark::State& state) {
    std::mt19937 rng(42);
    for (auto _ : state) {
        state.PauseTiming();
        nanomatch::OrderBook book(1 << 16);
        state.ResumeTiming();
        fill_limit_orders(book, static_cast<std::size_t>(state.range(0)), rng);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Optimized_AddLimit)->Range(1 << 10, 1 << 14);

static void BM_STL_AddLimit(benchmark::State& state) {
    std::mt19937 rng(42);
    for (auto _ : state) {
        state.PauseTiming();
        nanomatch::OrderBookStl book;
        state.ResumeTiming();
        fill_limit_orders_stl(book, static_cast<std::size_t>(state.range(0)), rng);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_STL_AddLimit)->Range(1 << 10, 1 << 14);

static void BM_Optimized_MatchBurst(benchmark::State& state) {
    nanomatch::OrderBook book(1 << 16);
    book.add_limit(1, nanomatch::Side::Sell, kBasePrice, 1000000, 0);
    for (auto _ : state) {
        book.clear_trades();
        for (int i = 0; i < state.range(0); ++i) {
            book.add_limit(static_cast<nanomatch::OrderId>(i + 100),
                           nanomatch::Side::Buy, kBasePrice, 1,
                           static_cast<nanomatch::Timestamp>(i));
        }
    }
}
BENCHMARK(BM_Optimized_MatchBurst)->Arg(1000)->Arg(10000);

BENCHMARK_MAIN();
