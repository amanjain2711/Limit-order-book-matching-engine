#include "nanomatch/order_book.hpp"
#include "nanomatch/order_book_stl.hpp"

#include <cassert>
#include <cstdio>

#define CHECK(cond)                                                                          \
    do {                                                                                     \
        if (!(cond)) {                                                                       \
            std::printf("FAIL: %s:%d %s\n", __FILE__, __LINE__, #cond);                      \
            return 1;                                                                        \
        }                                                                                    \
    } while (0)

int main() {
    nanomatch::OrderBook book(4096);

    CHECK(book.add_limit(1, nanomatch::Side::Buy, 100000, 100, 1));
    CHECK(book.add_limit(2, nanomatch::Side::Sell, 101000, 50, 2));
    CHECK(book.best_bid() == 100000);
    CHECK(book.best_ask() == 101000);

    CHECK(book.add_limit(3, nanomatch::Side::Buy, 101000, 30, 3));
    CHECK(book.trades().size() == 1);
    CHECK(book.trades()[0].qty == 30);
    CHECK(book.bid_depth(100000) == 100);

    CHECK(book.cancel(1));
    CHECK(book.bid_depth(100000) == 0);

    nanomatch::OrderBookStl stl;
    CHECK(stl.add_limit(10, nanomatch::Side::Buy, 50000, 10, 1));
    CHECK(stl.add_market(11, nanomatch::Side::Sell, 5, 2));
    CHECK(stl.trades().size() == 1);

    std::printf("All tests passed.\n");
    return 0;
}
