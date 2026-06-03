#pragma once

#include "nanomatch/memory_pool.hpp"
#include "nanomatch/order.hpp"
#include "nanomatch/types.hpp"

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

namespace nanomatch {

struct PriceLevel {
    Price price = 0;
    std::int32_t head = -1;
    std::int32_t tail = -1;
    Quantity total_qty = 0;
};

// Optimized LOB: contiguous pool + intrusive FIFO per price + sorted price index.
class OrderBook {
public:
    static constexpr std::size_t kDefaultPoolSize = 1 << 20;

    explicit OrderBook(std::size_t pool_capacity = kDefaultPoolSize);

    bool add_limit(OrderId id, Side side, Price price, Quantity qty, Timestamp ts);
    bool add_market(OrderId id, Side side, Quantity qty, Timestamp ts);
    bool cancel(OrderId id);

    Price best_bid() const;
    Price best_ask() const;
    Quantity bid_depth(Price p) const;
    Quantity ask_depth(Price p) const;

    std::size_t trade_count() const { return trades_.size(); }
    const std::vector<Trade>& trades() const { return trades_; }
    void clear_trades() { trades_.clear(); }

    std::size_t active_orders() const { return id_to_index_.size(); }

private:
    using TradeCallback = std::function<void(const Trade&)>;

    bool match_incoming(Side taker_side, Price limit_price, Quantity& qty,
                        OrderId taker_id, Timestamp ts, bool is_market);
    void match_against_level(PriceLevel& level, Side taker_side, Price limit_price,
                             Quantity& qty, OrderId taker_id, Timestamp ts, bool is_market);
    void append_to_level(PriceLevel& level, std::int32_t idx);
    void remove_from_level(PriceLevel& level, std::int32_t idx);
    PriceLevel* find_or_create_level(Side side, Price price);
    PriceLevel* level_for(Side side, Price price);
    const PriceLevel* level_for(Side side, Price price) const;
    void remove_empty_level(Side side, Price price);

    ObjectPool<OrderNode> pool_;
    std::unordered_map<OrderId, std::int32_t> id_to_index_;

    std::vector<PriceLevel> bids_;   // sorted descending by price
    std::vector<PriceLevel> asks_;   // sorted ascending by price
    std::vector<Trade> trades_;
};

}  // namespace nanomatch
