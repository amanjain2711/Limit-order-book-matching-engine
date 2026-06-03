#include "nanomatch/order_book_stl.hpp"

#include <algorithm>

namespace nanomatch {

bool OrderBookStl::add_limit(OrderId id, Side side, Price price, Quantity qty, Timestamp ts) {
    if (qty == 0 || id_lookup_.count(id)) {
        return false;
    }
    Quantity remaining = qty;
    if (!match_incoming(side, price, remaining, id, ts, false)) {
        return false;
    }
    if (remaining == 0) {
        return true;
    }
    SimpleOrder order{id, price, remaining, side, ts};
    if (side == Side::Buy) {
        bids_[price].push_back(order);
    } else {
        asks_[price].push_back(order);
    }
    id_lookup_[id] = {side, price};
    return true;
}

bool OrderBookStl::add_market(OrderId id, Side side, Quantity qty, Timestamp ts) {
    Quantity remaining = qty;
    return match_incoming(side, kInvalidPrice, remaining, id, ts, true);
}

bool OrderBookStl::cancel(OrderId id) {
    auto it = id_lookup_.find(id);
    if (it == id_lookup_.end()) {
        return false;
    }
    const auto [side, price] = it->second;
    if (side == Side::Buy) {
        auto& dq = bids_[price];
        dq.erase(std::remove_if(dq.begin(), dq.end(),
                                [id](const SimpleOrder& o) { return o.id == id; }),
                  dq.end());
        if (dq.empty()) {
            bids_.erase(price);
        }
    } else {
        auto& dq = asks_[price];
        dq.erase(std::remove_if(dq.begin(), dq.end(),
                                [id](const SimpleOrder& o) { return o.id == id; }),
                  dq.end());
        if (dq.empty()) {
            asks_.erase(price);
        }
    }
    id_lookup_.erase(it);
    return true;
}

Price OrderBookStl::best_bid() const {
    return bids_.empty() ? kInvalidPrice : bids_.begin()->first;
}

Price OrderBookStl::best_ask() const {
    return asks_.empty() ? kInvalidPrice : asks_.begin()->first;
}

bool OrderBookStl::match_incoming(Side taker_side, Price limit_price, Quantity& qty,
                                  OrderId taker_id, Timestamp ts, bool is_market) {
    if (taker_side == Side::Buy) {
        while (qty > 0 && !asks_.empty()) {
            auto ask_it = asks_.begin();
            if (!is_market && limit_price < ask_it->first) {
                break;
            }
            auto& queue = ask_it->second;
            while (qty > 0 && !queue.empty()) {
                auto& maker = queue.front();
                const Quantity fill = std::min(qty, maker.qty);
                trades_.push_back(Trade{maker.id, taker_id, ask_it->first, fill, ts});
                qty -= fill;
                maker.qty -= fill;
                if (maker.qty == 0) {
                    id_lookup_.erase(maker.id);
                    queue.pop_front();
                }
            }
            if (queue.empty()) {
                asks_.erase(ask_it);
            }
        }
    } else {
        while (qty > 0 && !bids_.empty()) {
            auto bid_it = bids_.begin();
            if (!is_market && limit_price > bid_it->first) {
                break;
            }
            auto& queue = bid_it->second;
            while (qty > 0 && !queue.empty()) {
                auto& maker = queue.front();
                const Quantity fill = std::min(qty, maker.qty);
                trades_.push_back(Trade{maker.id, taker_id, bid_it->first, fill, ts});
                qty -= fill;
                maker.qty -= fill;
                if (maker.qty == 0) {
                    id_lookup_.erase(maker.id);
                    queue.pop_front();
                }
            }
            if (queue.empty()) {
                bids_.erase(bid_it);
            }
        }
    }
    return true;
}

}  // namespace nanomatch
