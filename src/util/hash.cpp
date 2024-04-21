#include "hash.h"
#include "coding.h"

namespace ns_util {

#ifndef NO_SWITCH_BREAK_INTENDED
#define NO_SWITCH_BREAK_INTENDED \
    do {                         \
    } while (0)
#endif

uint32_t Hash(uint8_t const *data, uint64_t n, uint32_t seed) {
    // Similar to murmur hash
    uint32_t const m = 0xC6A4A793U;
    uint32_t const r = 24;
    uint8_t const *limit = data + n;
    uint32_t h = seed ^ (n * m);
    // Pick up four bytes at a time
    while (data + 4 <= limit) {
        uint32_t w = DecodeFixed32(data);
        data += 4;
        h += w;
        h *= m;
        h ^= (h >> 16);
    }
    // Pick up remaining bytes
    switch (limit - data) {
    case 3: {
        h += data[2] << 16;
        NO_SWITCH_BREAK_INTENDED;
    }
    case 2: {
        h += data[1] << 8;
        NO_SWITCH_BREAK_INTENDED;
    }
    case 1: {
        h += data[0];
        h *= m;
        h ^= (h >> r);
        break;
    }
    }
    return h;
}

} // ns_util