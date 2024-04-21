#ifndef _LEVEL_DB_XY_LOG_FORMAT_H_
#define _LEVEL_DB_XY_LOG_FORMAT_H_

#include <cstdint>

namespace ns_log {

enum RecordType {
    kZeroType = 0,

    kFullType = 1,

    kFirstType = 2,
    kMiddleType = 3,
    kLastType = 4
};

static constexpr int32_t kMaxRecordType = RecordType::kLastType;
static constexpr int32_t kBlockSize = 1 << 15;
// Header is checksum (4 bytes), length (2 bytes), type (1 byte).
static constexpr int32_t kHeaderSize = 4 + 2 + 1;
} // ns_log


#endif