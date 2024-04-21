#ifndef _LEVEL_DB_XY_LOG_READER_H_
#define _LEVEL_DB_XY_LOG_READER_H_

#include "status.h"
#include "env.h"
#include "log_format.h"
#include <cstdint>

namespace ns_log_reader {

class Reader {
public:
    class Reporter {
    public:
        virtual ~Reporter() = default;
        virtual void Corruption(uint64_t bytes, ns_util::Status const& status) = 0;
    };

    Reader(ns_env::SequentialFile* file, Reporter* reporter, bool checksum, uint64_t initial_offset);
    Reader(Reader const&) = delete;
    Reader& operator=(Reader const&) = delete;

    ~Reader();

    bool ReadRecord(ns_data_structure::Slice* record, std::string* scratch);

    uint64_t LastRecordOffset();

private:
    enum {
        kEof = ns_log::kMaxRecordType + 1,
        kBadRecord = ns_log::kMaxRecordType + 2
    };

    bool SkipToInitialBlock();
    uint32_t ReadPhysicalRecord(ns_data_structure::Slice* result);

    void ReportCorruption(uint64_t bytes, char const* reason);
    void ReportDrop(uint64_t bytes, ns_util::Status const& reason);

    ns_env::SequentialFile* const file_;
    Reporter* const reporter_;
    bool const checksum_;
    char* const backing_store_;
    ns_data_structure::Slice buffer_;
    bool eof_;

    uint64_t last_record_offset_;
    uint64_t end_of_buffer_offset_;
    uint64_t const initial_offset_;
    bool resyncing_;
};

} // ns_log_reader

#endif