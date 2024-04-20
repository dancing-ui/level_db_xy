#ifndef _LEVEL_DB_XY_CRC32C_H_
#define _LEVEL_DB_XY_CRC32C_H_

#include <cstdint>

namespace ns_util {
// Return the crc32c of concat(A, data[0,n-1]) where init_crc is the
// crc32c of some string A.  Extend() is often used to maintain the
// crc32c of a stream of data.
uint32_t Extend(uint32_t init_crc, uint8_t const *data, uint64_t n);

// Return the crc32c of data[0,n-1]
inline uint32_t Value(uint8_t const *data, uint64_t n) {
    return Extend(0, data, n);
}

static constexpr uint32_t kMaskDelta = 0xA282EAD8UL;
// Return a masked representation of crc.
//
// Motivation: it is problematic to compute the CRC of a string that
// contains embedded CRCs.  Therefore we recommend that CRCs stored
// somewhere (e.g., in files) should be masked before being stored.
inline uint32_t Mask(uint32_t crc) {
    // Rotate right by 15 bits and add a constant.
    return ((crc >> 15) | (crc << 17)) + kMaskDelta;
}

inline uint32_t Unmask(uint32_t masked_crc) {
    uint32_t rot = masked_crc - kMaskDelta;
    return ((rot >> 17) | (rot << 15));
}

} // ns_util

#endif