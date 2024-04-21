#ifndef _LEVEL_DB_XY_LOG_WRITER_H_
#define _LEVEL_DB_XY_LOG_WRITER_H_

#include "env.h"
#include "log_format.h"

namespace ns_log_writer {

class Writer {
public:
    explicit Writer(ns_env::WritableFile* dest);

    Writer(ns_env::WritableFile *dest, uint64_t dest_length);
    Writer(Writer const&) = delete;
    Writer& operator=(Writer const&) = delete;

    ~Writer() = default;
    ns_util::Status AddRecord(ns_data_structure::Slice const& slice);

private:
    ns_util::Status EmitPhysicalRecord(ns_log::RecordType type, uint8_t const* ptr, uint64_t length);

    ns_env::WritableFile* dest_;
    int32_t block_offset_;
    uint32_t type_crc_[ns_log::kMaxRecordType + 1];
};

} // ns_log_writer


#endif 