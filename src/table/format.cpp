#include "format.h"
#include "coding.h"
#include "crc32c.h"

namespace ns_table {

void BlockHandle::EncodeTo(std::string *dst) const {
    assert(offset_ != ~static_cast<uint64_t>(0));
    assert(size_ != ~static_cast<uint64_t>(0));
    ns_util::PutVarint64(dst, offset_);
    ns_util::PutVarint64(dst, size_);
}

ns_util::Status BlockHandle::DecodeFrom(ns_data_structure::Slice *input) {
    if (ns_util::GetVarint64(input, &offset_) && ns_util::GetVarint64(input, &size_)) {
        return ns_util::Status::OK();
    }
    return ns_util::Status::Corruption("bad block handle");
}

void Footer::EncodeTo(std::string *dst) const {
    uint64_t const original_size = dst->size();
    metaindex_handle_.EncodeTo(dst);
    index_handle_.EncodeTo(dst);
    dst->resize(2 * BlockHandle::kMaxEncodedLength);
    ns_util::PutFixed32(dst, static_cast<uint32_t>(kTableMagicNumber & 0xFFFFFFFFULL));
    ns_util::PutVarint32(dst, static_cast<uint32_t>(kTableMagicNumber >> 32));
    assert(dst->size() == original_size + kEncodedLength);
    static_cast<void>(original_size); // Disable unused variable warning.
}

ns_util::Status Footer::DecodeFrom(ns_data_structure::Slice *input) {
    if (input->size() < kEncodedLength) {
        return ns_util::Status::Corruption("not an sstable (footer too short)");
    }

    uint8_t const *magic_ptr = input->data() + kEncodedLength - 8;
    uint32_t const magic_lo = ns_util::DecodeFixed32(magic_ptr);
    uint32_t const magic_hi = ns_util::DecodeFixed32(magic_ptr + 4);
    uint64_t const magic = ((static_cast<uint64_t>(magic_hi) << 32) | static_cast<uint64_t>(magic_lo));

    if (magic != kTableMagicNumber) {
        return ns_util::Status::Corruption("not an sstable (bad magic number)");
    }

    ns_util::Status result = metaindex_handle_.DecodeFrom(input);
    if (result.ok()) {
        result = index_handle_.DecodeFrom(input);
    }
    if (result.ok()) {
        // We skip over any leftover data (just padding for now) in "input"
        uint8_t const *end = magic_ptr + 8;
        *input = ns_data_structure::Slice(end, input->data() + input->size() - end);
    }
    return result;
}

ns_util::Status ReadBlock(ns_env::RandomAccessFile *file, ns_options::ReadOptions const &options,
                          BlockHandle const &handle, BlockContents *result) {
    result->data = ns_data_structure::Slice();
    result->cachable = false;
    result->heap_allocated = false;
    // Read the block contents as well as the type/crc footer.
    // See table_builder.cc for the code that built this structure.
    uint64_t n = static_cast<uint64_t>(handle.size());
    char *buf = new char[n + kBlockTrailerSize];
    ns_data_structure::Slice contents;
    ns_util::Status s = file->Read(handle.offset(), n + kBlockTrailerSize, &contents, buf);
    if (!s.ok()) {
        delete[] buf;
        return s;
    }
    if (contents.size() != n + kBlockTrailerSize) {
        delete[] buf;
        return ns_util::Status::Corruption("truncated block read");
    }
    // Check the crc of the type and the block contents
    uint8_t const *data = contents.data(); // Pointer to where Read put the data
    if (options.verify_checksums) {
        uint32_t const crc = ns_util::Unmask(ns_util::DecodeFixed32(data + n + 1));
        uint32_t const actual = ns_util::Value(data, n + 1);
        if (actual != crc) {
            delete[] buf;
            s = ns_util::Status::Corruption("block checksum mismatch");
            return s;
        }
    }

    switch (data[n]) {
    case ns_compression::CompressionType::kNoCompression: {
        if (data != reinterpret_cast<uint8_t const *>(buf)) {
            // File implementation gave us pointer to some other data.
            // Use it directly under the assumption that it will be live
            // while the file is open.
            delete[] buf;
            result->data = ns_data_structure::Slice(data, n);
            result->heap_allocated = false;
            result->cachable = false; // Do not double-cache
        } else {
            result->data = ns_data_structure::Slice(buf, n);
            result->heap_allocated = true;
            result->cachable = true;
        }
        // Ok
        break;
    }
    case ns_compression::CompressionType::kSnappyCompression: {
        uint64_t ulength = 0;
        if (!ns_compression::Snappy_GetUncompressedLength(reinterpret_cast<char const *>(data), n, &ulength)) {
            delete[] buf;
            return ns_util::Status::Corruption("corrupted snappy compressed block length");
        }
        char *ubuf = new char[ulength];
        if (!ns_compression::Snappy_Uncompress(reinterpret_cast<char const *>(data), n, ubuf)) {
            delete[] buf;
            delete[] ubuf;
            return ns_util::Status::Corruption("corrupted snappy compressed block contents");
        }
        delete[] buf;
        result->data = ns_data_structure::Slice(ubuf, ulength);
        result->heap_allocated = true;
        result->cachable = true;
        break;
    }
    case ns_compression::CompressionType::kZstdCompression: {
        uint64_t ulength = 0;
        if (!ns_compression::Zstd_GetUncompressedLength(reinterpret_cast<char const *>(data), n, &ulength)) {
            delete[] buf;
            return ns_util::Status::Corruption("corrupted zstd compressed block length");
        }
        char *ubuf = new char[ulength];
        if (!ns_compression::Zstd_Uncompress(reinterpret_cast<char const *>(data), n, ubuf)) {
            delete[] buf;
            delete[] ubuf;
            return ns_util::Status::Corruption("corrupted zstd compressed block contents");
        }
        delete[] buf;
        result->data = ns_data_structure::Slice(ubuf, ulength);
        result->heap_allocated = true;
        result->cachable = true;
        break;
    }
    default: {
        delete[] buf;
        return ns_util::Status::Corruption("bad block type");
    }
    }
    return ns_util::Status::OK();
}

} // ns_table