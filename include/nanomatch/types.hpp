#pragma once

#include <cstdint>
#include <limits>

namespace nanomatch {

using OrderId = std::uint64_t;
using Price = std::int64_t;   // fixed-point: price * 10000 (e.g. $10.25 -> 102500)
using Quantity = std::uint32_t;
using Timestamp = std::uint64_t;

constexpr Price kInvalidPrice = std::numeric_limits<Price>::min();

enum class Side : std::uint8_t { Buy = 0, Sell = 1 };

enum class OrderType : std::uint8_t {
    Limit = 0,
    Market = 1,
    Cancel = 2
};

inline const char* side_name(Side s) {
    return s == Side::Buy ? "BUY" : "SELL";
}

}  // namespace nanomatch
