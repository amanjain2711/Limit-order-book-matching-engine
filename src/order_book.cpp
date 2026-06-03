#include "nanomatch/order_book.hpp"

#include <algorithm>
#include <cassert>

namespace nanomatch {

namespace {

auto book_side(std::vector<PriceLevel>& bids, std::vector<PriceLevel>& asks, Side s)
    -> std::vector<PriceLevel>& {
    return s == Side::Buy ? bids : asks;
}

auto book_side(const std::vector<PriceLevel>& bids, const std::vector<PriceLevel>& asks, Side s)
    -> const std::vector<PriceLevel>& {
    return s == Side::Buy ? bids : asks;
}

}  // namespace

OrderBook::OrderBook(std::size_t pool_capacity) : pool_(pool_capacity) {}

bool OrderBook::add_limit(OrderId id, Side side, Price price, Quantity qty, Timestamp ts) {
    if (qty == 0 || id_to_index_.count(id)) {
        return false;
    }
    Quantity remaining = qty;
    if (!match_incoming(side, price, remaining, id, ts, false)) {
        return false;
    }
    if (remaining == 0) {
        return true;
    }
    const std::int32_t idx = pool_.allocate();
    if (idx < 0) {
        return false;
    }
    auto& node = pool_.get(idx);
    node.id = id;
    node.price = price;
    node.qty = remaining;
    node.side = side;
    node.ts = ts;
    id_to_index_[id] = idx;
    PriceLevel* level = find_or_create_level(side, price);
    append_to_level(*level, idx);
    return true;
}

bool OrderBook::add_market(OrderId id, Side side, Quantity qty, Timestamp ts) {
    if (qty == 0) {
        return false;
    }
    Quantity remaining = qty;
    return match_incoming(side, kInvalidPrice, remaining, id, ts, true);
}

bool OrderBook::cancel(OrderId id) {
    auto it = id_to_index_.find(id);
    if (it == id_to_index_.end()) {
        return false;
    }
    const std::int32_t idx = it->second;
    auto& node = pool_.get(idx);
    PriceLevel* level = level_for(node.side, node.price);
    if (level) {
        remove_from_level(*level, idx);
        if (level->head < 0) {
            remove_empty_level(node.side, node.price);
        }
    }
    pool_.deallocate(idx);
    id_to_index_.erase(it);
    return true;
}

Price OrderBook::best_bid() const {
  return bids_.empty() ? kInvalidPrice : bids_.front().price;
}

Price OrderBook::best_ask() const {
  return asks_.empty() ? kInvalidPrice : asks_.front().price;
}

Quantity OrderBook::bid_depth(Price p) const {
    const PriceLevel* lvl = level_for(Side::Buy, p);
    return lvl ? lvl->total_qty : 0;
}

Quantity OrderBook::ask_depth(Price p) const {
    const PriceLevel* lvl = level_for(Side::Sell, p);
    return lvl ? lvl->total_qty : 0;
}

bool OrderBook::match_incoming(Side taker_side, Price limit_price, Quantity& qty,
                               OrderId taker_id, Timestamp ts, bool is_market) {
    Side book_side_opposite = taker_side == Side::Buy ? Side::Sell : Side::Buy;
    auto& contra = book_side(bids_, asks_, book_side_opposite);

    while (qty > 0 && !contra.empty()) {
        PriceLevel& top = contra.front();
        if (!is_market) {
            if (taker_side == Side::Buy && limit_price < top.price) {
                break;
            }
            if (taker_side == Side::Sell && limit_price > top.price) {
                break;
            }
        }
        match_against_level(top, taker_side, limit_price, qty, taker_id, ts, is_market);
        if (top.head < 0) {
            contra.erase(contra.begin());
        }
    }
    return true;
}

void OrderBook::match_against_level(PriceLevel& level, Side taker_side, Price limit_price,
                                    Quantity& qty, OrderId taker_id, Timestamp ts,
                                    bool is_market) {
    (void)limit_price;
    (void)is_market;
    while (qty > 0 && level.head >= 0) {
        const std::int32_t idx = level.head;
        auto& maker = pool_.get(idx);
        const Quantity fill = std::min(qty, maker.qty);

        trades_.push_back(Trade{
            maker.id,
            taker_id,
            level.price,
            fill,
            ts,
        });

        qty -= fill;
        maker.qty -= fill;
        level.total_qty -= fill;

        if (maker.qty == 0) {
            level.head = maker.next;
            if (level.head >= 0) {
                pool_.get(level.head).prev = -1;
            } else {
                level.tail = -1;
            }
            id_to_index_.erase(maker.id);
            pool_.deallocate(idx);
        }
    }
}

void OrderBook::append_to_level(PriceLevel& level, std::int32_t idx) {
    auto& node = pool_.get(idx);
    node.prev = level.tail;
    node.next = -1;
    if (level.tail >= 0) {
        pool_.get(level.tail).next = idx;
    } else {
        level.head = idx;
    }
    level.tail = idx;
    level.total_qty += node.qty;
}

void OrderBook::remove_from_level(PriceLevel& level, std::int32_t idx) {
    auto& node = pool_.get(idx);
    level.total_qty -= node.qty;
    if (node.prev >= 0) {
        pool_.get(node.prev).next = node.next;
    } else {
        level.head = node.next;
    }
    if (node.next >= 0) {
        pool_.get(node.next).prev = node.prev;
    } else {
        level.tail = node.prev;
    }
}

PriceLevel* OrderBook::find_or_create_level(Side side, Price price) {
    auto& levels = book_side(bids_, asks_, side);
    if (side == Side::Buy) {
        auto it = std::lower_bound(levels.begin(), levels.end(), price,
                                  [](const PriceLevel& lvl, Price p) { return lvl.price > p; });
        if (it != levels.end() && it->price == price) {
            return &(*it);
        }
        it = levels.insert(it, PriceLevel{price, -1, -1, 0});
        return &(*it);
    }
    auto it = std::lower_bound(levels.begin(), levels.end(), price,
                              [](const PriceLevel& lvl, Price p) { return lvl.price < p; });
    if (it != levels.end() && it->price == price) {
        return &(*it);
    }
    it = levels.insert(it, PriceLevel{price, -1, -1, 0});
    return &(*it);
}

PriceLevel* OrderBook::level_for(Side side, Price price) {
    auto& levels = book_side(bids_, asks_, side);
    for (auto& lvl : levels) {
        if (lvl.price == price) {
            return &lvl;
        }
    }
    return nullptr;
}

const PriceLevel* OrderBook::level_for(Side side, Price price) const {
    const auto& levels = book_side(bids_, asks_, side);
    for (const auto& lvl : levels) {
        if (lvl.price == price) {
            return &lvl;
        }
    }
    return nullptr;
}

void OrderBook::remove_empty_level(Side side, Price price) {
    auto& levels = book_side(bids_, asks_, side);
    levels.erase(
        std::remove_if(levels.begin(), levels.end(),
                       [price](const PriceLevel& lvl) { return lvl.price == price; }),
        levels.end());
}

}  // namespace nanomatch
