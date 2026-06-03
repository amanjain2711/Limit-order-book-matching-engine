#pragma once

#include "nanomatch/types.hpp"

#include <cstddef>

namespace nanomatch {

// Cache-line aligned order node for intrusive FIFO queues at each price level.
struct alignas(64) OrderNode {
    OrderId id = 0;
    Price price = 0;
    Quantity qty = 0;
    Side side = Side::Buy;
    Timestamp ts = 0;

    std::int32_t prev = -1;  // pool indices
    std::int32_t next = -1;
    bool active = false;
};

static_assert(sizeof(OrderNode) >= 64, "OrderNode should occupy at least one cache line");

struct Trade {
    OrderId maker_id = 0;
    OrderId taker_id = 0;
    Price price = 0;
    Quantity qty = 0;
    Timestamp ts = 0;
};

}  // namespace nanomatch
