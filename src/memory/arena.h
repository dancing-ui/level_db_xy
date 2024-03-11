#ifndef _LEVEL_DB_XY_ARENA_H_
#define _LEVEL_DB_XY_ARENA_H_

#include <cstdint>
#include <vector>
#include <atomic>

namespace ns_memory {
namespace ns_arena {

class Arena {
public:
    Arena();
    ~Arena();

    uint8_t *Allocate(uint64_t bytes);
    uint8_t *AllocateAligned(uint64_t bytes);
    uint64_t MemoryUsage() const;

private:
    uint8_t *AllocateFallback(uint64_t bytes);
    uint8_t *AllocateNewBlock(uint64_t block_bytes);

    uint8_t *alloc_ptr_{nullptr};
    uint64_t alloc_bytes_remaining_{0UL};
    std::vector<uint8_t *> blocks_;
    std::atomic<uint64_t> memory_usage_;
};

} // ns_arena
} // ns_memory

#endif