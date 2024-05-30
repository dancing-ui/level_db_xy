#ifndef _LEVEL_DB_XY_FORMAT_H_
#define _LEVEL_DB_XY_FORMAT_H_

#include "slice.h"
#include "status.h"
#include "env.h"
#include "options.h"

namespace ns_table {

struct BlockContents {
    ns_data_structure::Slice data; // Actual contents of data
    bool cachable;                 // True iff data can be cached
    bool heap_allocated;           // True iff caller should delete[] data.data()
};

class BlockHandle {
public:
    // Maximum encoding length of a BlockHandle
    enum { kMaxEncodedLength = 10 + 10 };

    BlockHandle() :
        offset_(~static_cast<uint64_t>(0)),
        size_(~static_cast<uint64_t>(0)) {
    }

    uint64_t offset() const {
        return offset_;
    }

    void set_offset(uint64_t offset) {
        offset_ = offset;
    }

    uint64_t size() const {
        return size_;
    }

    void set_size(uint64_t size) {
        size_ = size;
    }

    void EncodeTo(std::string *dst) const;

    ns_util::Status DecodeFrom(ns_data_structure::Slice *input);

private:
    uint64_t offset_;
    uint64_t size_;
};

// 1-byte type + 32-bit crc
static constexpr uint64_t kBlockTrailerSize{5UL};

// Footer encapsulates the fixed information stored at the tail
// end of every table file.
class Footer {
public:
    enum { kEncodedLength = 2 * BlockHandle::kMaxEncodedLength + 8 };

    Footer() = default;

    BlockHandle const &metaindex_handle() const {
        return metaindex_handle_;
    }

    void set_metaindex_handle(BlockHandle const &h) {
        metaindex_handle_ = h;
    }

    BlockHandle const &index_handle() const {
        return index_handle_;
    }

    void set_index_handle(BlockHandle const &h) {
        index_handle_ = h;
    }

    void EncodeTo(std::string *dst) const;

    ns_util::Status DecodeFrom(ns_data_structure::Slice *input);

private:
    BlockHandle metaindex_handle_;
    BlockHandle index_handle_;
};
// kTableMagicNumber was picked by running
//    echo http://code.google.com/p/leveldb/ | sha1sum
// and taking the leading 64 bits.
static constexpr uint64_t kTableMagicNumber{0xDB4775248B80FB57ULL};

// Read the block identified by "handle" from "file".  On failure
// return non-OK.  On success fill *result and return OK.
ns_util::Status ReadBlock(ns_env::RandomAccessFile *file, ns_options::ReadOptions const &options,
                          BlockHandle const &handle, BlockContents *result);

} // ns_table

#endif