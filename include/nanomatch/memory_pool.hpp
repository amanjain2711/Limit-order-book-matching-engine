#pragma once

#include "nanomatch/order.hpp"

#include <cassert>
#include <cstddef>
#include <vector>

namespace nanomatch {

// Contiguous slab allocator for OrderNode — no per-order heap allocation.
template <typename T>
class ObjectPool {
public:
    explicit ObjectPool(std::size_t capacity)
        : storage_(capacity), free_head_(0) {
        for (std::size_t i = 0; i < capacity; ++i) {
            storage_[i].next = static_cast<std::int32_t>(i + 1);
        }
        if (capacity > 0) {
            storage_[capacity - 1].next = -1;
        }
    }

    std::int32_t allocate() {
        if (free_head_ < 0) {
            return -1;
        }
        const std::int32_t idx = free_head_;
        free_head_ = storage_[static_cast<std::size_t>(idx)].next;
        storage_[static_cast<std::size_t>(idx)].next = -1;
        storage_[static_cast<std::size_t>(idx)].prev = -1;
        storage_[static_cast<std::size_t>(idx)].active = true;
        return idx;
    }

    void deallocate(std::int32_t idx) {
        assert(idx >= 0 && static_cast<std::size_t>(idx) < storage_.size());
        auto& node = storage_[static_cast<std::size_t>(idx)];
        node.active = false;
        node.prev = -1;
        node.next = free_head_;
        free_head_ = idx;
    }

    T& get(std::int32_t idx) { return storage_[static_cast<std::size_t>(idx)]; }
    const T& get(std::int32_t idx) const { return storage_[static_cast<std::size_t>(idx)]; }

    std::size_t capacity() const { return storage_.size(); }
    std::size_t size_active() const { return capacity() - free_count(); }

private:
    std::size_t free_count() const {
        std::size_t count = 0;
        for (std::int32_t i = free_head_; i >= 0;) {
            ++count;
            i = storage_[static_cast<std::size_t>(i)].next;
        }
        return count;
    }

    std::vector<T> storage_;
    std::int32_t free_head_;
};

}  // namespace nanomatch
