#pragma once

#include "nanomatch/order.hpp"
#include "nanomatch/types.hpp"

#include <deque>
#include <map>
#include <unordered_map>
#include <vector>

namespace nanomatch {

// Baseline reference implementation using STL containers (for benchmark comparison).
class OrderBookStl {
public:
    struct SimpleOrder {
        OrderId id = 0;
        Price price = 0;
        Quantity qty = 0;
        Side side = Side::Buy;
        Timestamp ts = 0;
    };

    bool add_limit(OrderId id, Side side, Price price, Quantity qty, Timestamp ts);
    bool add_market(OrderId id, Side side, Quantity qty, Timestamp ts);
    bool cancel(OrderId id);

    Price best_bid() const;
    Price best_ask() const;

    const std::vector<Trade>& trades() const { return trades_; }
    void clear_trades() { trades_.clear(); }

private:
    bool match_incoming(Side taker_side, Price limit_price, Quantity& qty,
                        OrderId taker_id, Timestamp ts, bool is_market);

    std::map<Price, std::deque<SimpleOrder>, std::greater<Price>> bids_;
    std::map<Price, std::deque<SimpleOrder>> asks_;
    std::unordered_map<OrderId, std::pair<Side, Price>> id_lookup_;
    std::vector<Trade> trades_;
};

}  // namespace nanomatch
