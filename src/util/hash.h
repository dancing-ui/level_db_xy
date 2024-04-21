#ifndef _LEVEL_DB_XY_HASH_H_
#define _LEVEL_DB_XY_HASH_H_

#include <cstdint>

namespace ns_util {

uint32_t Hash(uint8_t const* data, uint64_t n, uint32_t seed);

} // ns_util

#endif