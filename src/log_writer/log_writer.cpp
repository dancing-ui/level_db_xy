#include "log_writer.h"
#include "coding.h"
#include "crc32c.h"
#include "log_format.h"

namespace ns_log_writer {

static void InitTypeCrc(uint32_t *type_crc) {
    for (int32_t i = 0; i <= ns_log::kMaxRecordType; i++) {
        uint8_t t = static_cast<uint8_t>(i);
        type_crc[i] = ns_util::Value(&t, 1);
    }
}

Writer::Writer(ns_env::WritableFile *dest) :
    dest_(dest), block_offset_(0) {
    InitTypeCrc(type_crc_);
}

Writer::Writer(ns_env::WritableFile *dest, uint64_t dest_length) :
    dest_(dest), block_offset_(dest_length % ns_log::kBlockSize) {
    InitTypeCrc(type_crc_);
}

ns_util::Status Writer::AddRecord(ns_data_structure::Slice const &slice) {
    uint8_t const *ptr = slice.data();
    uint64_t left = slice.size();
    ns_util::Status s;
    bool begin = true;
    do {
        int32_t const left_over = ns_log::kBlockSize - block_offset_;
        assert(left_over >= 0);
        if (left_over < ns_log::kHeaderSize) {
            if (left_over > 0) {
                static_assert(ns_log::kHeaderSize == 7, "");
                dest_->Append(ns_data_structure::Slice("\x00\x00\x00\x00\x00\x00", left_over));
            }
            block_offset_ = 0;
        }
        assert(ns_log::kBlockSize - block_offset_ - ns_log::kHeaderSize >= 0);
        uint64_t const avail = ns_log::kBlockSize - block_offset_ - ns_log::kHeaderSize;
        uint64_t const fragment_length = (left < avail) ? left : avail;
        ns_log::RecordType type;
        bool const end = (left == fragment_length);
        if (begin && end) {
            type = ns_log::RecordType::kFullType;
        } else if (begin) {
            type = ns_log::RecordType::kFirstType;
        } else if (end) {
            type = ns_log::RecordType::kLastType;
        } else {
            type = ns_log::RecordType::kMiddleType;
        }
        s = EmitPhysicalRecord(type, ptr, fragment_length);
        ptr += fragment_length;
        left -= fragment_length;
        begin = false;
    } while (s.ok() && left > 0);
    return s;
}
ns_util::Status Writer::EmitPhysicalRecord(ns_log::RecordType type, uint8_t const *ptr, uint64_t length) {
    assert(length <= 0xFFFFU);
    assert(block_offset_ + ns_log::kHeaderSize + length <= ns_log::kBlockSize);

    // Format the header
    uint8_t buf[ns_log::kHeaderSize];
    buf[4] = length & 0xFFU;
    buf[5] = length >> 8;
    buf[6] = type;

    // generate the crc32
    uint32_t crc = ns_util::Extend(type_crc_[type], ptr, length);
    crc = ns_util::Mask(crc);
    ns_util::EncodeFixed32(buf, crc);

    // Write the header and the payload
    ns_util::Status s = dest_->Append(ns_data_structure::Slice(buf, ns_log::kHeaderSize));
    if (s.ok()) {
        s = dest_->Append(ns_data_structure::Slice(ptr, length));
        if (s.ok()) {
            s = dest_->Flush();
        }
    }
    block_offset_ += ns_log::kHeaderSize + length;
    return s;
}

} // ns_log_writer