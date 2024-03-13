#include "arena.h"
#include "log.h"
#include <cassert>

namespace ns_memory {

static constexpr int32_t kBlockSize{4096};

Arena::Arena() {
    alloc_ptr_ = nullptr;
    alloc_bytes_remaining_ = 0UL;
    memory_usage_ = 0UL;
}
Arena::~Arena() {
    for (uint64_t i = 0UL; i < blocks_.size(); i++) {
        delete[] blocks_[i];
    }
}
uint8_t *Arena::Allocate(uint64_t bytes) {
    assert(bytes > 0);
    if (bytes <= alloc_bytes_remaining_) {
        uint8_t *result = alloc_ptr_;
        alloc_ptr_ += bytes;
        alloc_bytes_remaining_ -= bytes;
        return result;
    }
    return AllocateFallback(bytes);
}
uint8_t *Arena::AllocateAligned(uint64_t bytes) {
    constexpr int32_t align = (sizeof(void *) > 8) ? sizeof(void *) : 8;
    static_assert((align & (align - 1)) == 0,
                  "Pointer size should be a power of 2");
    uint64_t current_mod = reinterpret_cast<uintptr_t>(alloc_ptr_) & (align - 1);
    uint64_t slop = (current_mod == 0 ? 0 : align - current_mod);
    uint64_t needed = bytes + slop;
    uint8_t *result{nullptr};
    if (needed <= alloc_bytes_remaining_) {
        result = alloc_ptr_ + slop;
        alloc_ptr_ += needed;
        alloc_bytes_remaining_ -= needed;
    } else {
        result = AllocateFallback(bytes);
    }
    assert((reinterpret_cast<uintptr_t>(result) & (align - 1)) == 0);
    return result;
}
uint64_t Arena::MemoryUsage() const {
    return memory_usage_.load(std::memory_order_relaxed);
}
uint8_t *Arena::AllocateFallback(uint64_t bytes) {
    if (bytes > (kBlockSize >> 2)) {
        uint8_t *result = AllocateNewBlock(bytes);
        return result;
    }
    alloc_ptr_ = AllocateNewBlock(kBlockSize);
    alloc_bytes_remaining_ = kBlockSize;
    uint8_t *result = alloc_ptr_;
    alloc_ptr_ += bytes;
    alloc_bytes_remaining_ -= bytes;
    return result;
}
uint8_t *Arena::AllocateNewBlock(uint64_t block_bytes) {
    uint8_t *result = new uint8_t[block_bytes];
    blocks_.emplace_back(result);
    memory_usage_.fetch_add(block_bytes + sizeof(uint8_t*), std::memory_order_relaxed);
    return result;
}

} // ns_memory