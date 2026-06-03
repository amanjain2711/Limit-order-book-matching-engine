#pragma once

#include "nanomatch/types.hpp"

#include <string>
#include <vector>

namespace nanomatch {

struct MarketEvent {
    OrderType type = OrderType::Limit;
    OrderId id = 0;
    Side side = Side::Buy;
    Price price = 0;
    Quantity qty = 0;
    Timestamp ts = 0;
};

// Loads market events from CSV (portable). On Linux, mmap variant can be added for zero-copy.
std::vector<MarketEvent> load_csv(const std::string& path);

}  // namespace nanomatch
