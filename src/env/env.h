#ifndef _LEVEL_DB_XY_ENV_H_
#define _LEVEL_DB_XY_ENV_H_

#include "status.h"
#include "slice.h"

namespace ns_env {

class WritableFile {
public:
    WritableFile() = default;

    WritableFile(WritableFile const&) = delete;
    WritableFile& operator=(WritableFile const&) = delete;

    virtual ~WritableFile() = default;

    virtual ns_util::Status Append(ns_data_structure::Slice const& data) = 0;
    virtual ns_util::Status Close() = 0;
    virtual ns_util::Status Flush() = 0;
    virtual ns_util::Status Sync() = 0;
};

class SequentialFile {
public:
    SequentialFile() = default;

    SequentialFile(SequentialFile const&) = delete;
    SequentialFile& operator=(SequentialFile const&) = delete;

    virtual ~SequentialFile() = default;
    virtual ns_util::Status Read(uint64_t n, ns_data_structure::Slice* result, char * scratch) = 0;
    virtual ns_util::Status Skip(uint64_t n) = 0;
};

} // ns_env

#endif