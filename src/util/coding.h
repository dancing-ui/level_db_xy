#ifndef _LEVEL_DB_XY_CODING_H_
#define _LEVEL_DB_XY_CODING_H_

#include <cstdint>
#include <string>

namespace ns_util {

void PutFixed64(std::string *dst, uint64_t value);

uint8_t *EncodeVarint32(uint8_t *dst, uint32_t value);

uint8_t const *GetVarint32Ptr(uint8_t const *p, uint8_t const *limit, uint32_t *value);

inline uint64_t DecodeFixed64(uint8_t const *ptr) {
    return (static_cast<uint64_t>(ptr[0])) | (static_cast<uint64_t>(ptr[1]) << 8) | (static_cast<uint64_t>(ptr[2]) << 16) | (static_cast<uint64_t>(ptr[3]) << 24) | (static_cast<uint64_t>(ptr[4]) << 32) | (static_cast<uint64_t>(ptr[5]) << 40) | (static_cast<uint64_t>(ptr[6]) << 48) | (static_cast<uint64_t>(ptr[7]) << 56);
}

inline void EncodeFixed64(uint8_t *dst, uint64_t value) {
    dst[0] = static_cast<uint8_t>(value);
    dst[1] = static_cast<uint8_t>(value >> 8);
    dst[2] = static_cast<uint8_t>(value >> 16);
    dst[3] = static_cast<uint8_t>(value >> 24);
    dst[4] = static_cast<uint8_t>(value >> 32);
    dst[5] = static_cast<uint8_t>(value >> 40);
    dst[6] = static_cast<uint8_t>(value >> 48);
    dst[7] = static_cast<uint8_t>(value >> 56);
}

} // ns_util

#endif