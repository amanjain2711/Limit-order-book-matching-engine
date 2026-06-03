#pragma once

#include "nanomatch/order.hpp"

#include <atomic>
#include <cstddef>
#include <optional>

namespace nanomatch {

// Single-Producer Single-Consumer lock-free ring buffer for trade logging.
template <typename T, std::size_t Capacity>
class SpscRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of two");

public:
    bool try_push(const T& item) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t next = (head + 1) & mask_;
        if (next == tail_.load(std::memory_order_acquire)) {
            return false;  // full
        }
        buffer_[head] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    std::optional<T> try_pop() {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt;  // empty
        }
        T item = buffer_[tail];
        tail_.store((tail + 1) & mask_, std::memory_order_release);
        return item;
    }

    std::size_t size_approx() const {
        const std::size_t h = head_.load(std::memory_order_acquire);
        const std::size_t t = tail_.load(std::memory_order_acquire);
        return (h + Capacity - t) & mask_;
    }

private:
    static constexpr std::size_t mask_ = Capacity - 1;
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};  // separate cache lines — avoid false sharing
    T buffer_[Capacity]{};
};

}  // namespace nanomatch
