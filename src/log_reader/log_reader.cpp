#include "log_reader.h"
#include "crc32c.h"
#include "coding.h"

namespace ns_log_reader {

Reader::Reader(ns_env::SequentialFile *file, Reporter *reporter, bool checksum, uint64_t initial_offset) :
    file_(file), reporter_(reporter), checksum_(checksum),
    backing_store_(new char[ns_log::kBlockSize]), buffer_(), eof_(false),
    last_record_offset_(0U), end_of_buffer_offset_(0U), initial_offset_(initial_offset),
    resyncing_(initial_offset > 0U) {
}

Reader::~Reader() {
    delete[] backing_store_;
}

bool Reader::ReadRecord(ns_data_structure::Slice *record, std::string *scratch) {
    if (last_record_offset_ < initial_offset_) {
        if (!SkipToInitialBlock()) {
            return false;
        }
    }
    scratch->clear();
    record->clear();
    bool in_fragmented_record = false;
    uint64_t prospective_record_offset = 0U;
    ns_data_structure::Slice fragment;
    while (true) {
        uint32_t const record_type = ReadPhysicalRecord(&fragment);
        uint64_t physical_record_offset = end_of_buffer_offset_ - buffer_.size() - ns_log::kHeaderSize - fragment.size();
        if (resyncing_) {
            if (record_type == ns_log::RecordType::kMiddleType) {
                continue;
            } else if (record_type == ns_log::RecordType::kLastType) {
                resyncing_ = false;
                continue;
            } else {
                resyncing_ = false;
            }
        }

        switch (record_type) {
        case ns_log::RecordType::kFullType: {
            if (in_fragmented_record) {
                if (!scratch->empty()) {
                    ReportCorruption(scratch->size(), "partial record without end(1)");
                }
            }
            prospective_record_offset = physical_record_offset;
            scratch->clear();
            *record = fragment;
            last_record_offset_ = prospective_record_offset;
            return true;
        }
        case ns_log::RecordType::kFirstType: {
            if (in_fragmented_record) {
                if (!scratch->empty()) {
                    ReportCorruption(scratch->size(), "partial record without end(2)");
                }
            }
            prospective_record_offset = physical_record_offset;
            scratch->assign(reinterpret_cast<char const *>(fragment.data()), fragment.size());
            in_fragmented_record = true;
            break;
        }
        case ns_log::RecordType::kMiddleType: {
            if (!in_fragmented_record) {
                ReportCorruption(fragment.size(), "missing start of fragmented record(1)");
            } else {
                scratch->append(reinterpret_cast<char const *>(fragment.data()), fragment.size());
            }
            break;
        }
        case ns_log::RecordType::kLastType: {
            if (!in_fragmented_record) {
                ReportCorruption(fragment.size(), "missing start of fragmented record(2)");
            } else {
                scratch->append(reinterpret_cast<char const *>(fragment.data()), fragment.size());
                *record = ns_data_structure::Slice(*scratch);
                last_record_offset_ = prospective_record_offset;
                return true;
            }
            break;
        }
        case kEof: {
            if (in_fragmented_record) {
                scratch->clear();
            }
            return false;
        }
        case kBadRecord: {
            if (in_fragmented_record) {
                ReportCorruption(scratch->size(), "error in middle of record");
                in_fragmented_record = false;
                scratch->clear();
            }
            break;
        }
        default: {
            char buf[40];
            std::snprintf(buf, sizeof(buf), "unknown record type %u", record_type);
            ReportCorruption((fragment.size() + (in_fragmented_record ? scratch->size() : 0)), buf);
            in_fragmented_record = false;
            scratch->clear();
            break;
        }
        }
    }
    return false;
}

uint64_t Reader::LastRecordOffset() {
    return last_record_offset_;
}

bool Reader::SkipToInitialBlock() {
    uint64_t const offset_in_block = initial_offset_ % ns_log::kBlockSize;
    uint64_t blcok_start_location = initial_offset_ - offset_in_block;

    if (offset_in_block > ns_log::kBlockSize - 6) {
        blcok_start_location += ns_log::kBlockSize;
    }

    end_of_buffer_offset_ = blcok_start_location;
    // Skip to start of first block that can contain the initial record
    if (blcok_start_location > 0) {
        ns_util::Status skip_status = file_->Skip(blcok_start_location);
        if (!skip_status.ok()) {
            ReportDrop(blcok_start_location, skip_status);
            return false;
        }
    }
    return true;
}

uint32_t Reader::ReadPhysicalRecord(ns_data_structure::Slice *result) {
    while (true) {
        if (buffer_.size() < ns_log::kHeaderSize) {
            if (!eof_) {
                // Last read was a full read, so this is a trailer to skip
                buffer_.clear();
                ns_util::Status status = file_->Read(ns_log::kBlockSize, &buffer_, backing_store_);
                end_of_buffer_offset_ += buffer_.size();
                if (!status.ok()) {
                    buffer_.clear();
                    ReportDrop(ns_log::kBlockSize, status);
                    eof_ = true;
                    return kEof;
                } else if (buffer_.size() < ns_log::kBlockSize) {
                    eof_ = true;
                }
                continue;
            } else {
                // Note that if buffer_ is non-empty, we have a truncated header at the
                // end of the file, which can be caused by the writer crashing in the
                // middle of writing the header. Instead of considering this an error,
                // just report EOF.
                buffer_.clear();
                return kEof;
            }
        }

        uint8_t const *header = buffer_.data();
        uint32_t const length = (static_cast<uint32_t>(header[5]) << 8) | header[4];
        uint32_t const type = header[6];
        if (ns_log::kHeaderSize + length > buffer_.size()) {
            uint64_t drop_size = buffer_.size();
            buffer_.clear();
            if (!eof_) {
                ReportCorruption(drop_size, "bad record length");
                return kBadRecord;
            }
            // If the end of the file has been reached without reading |length| bytes
            // of payload, assume the writer died in the middle of writing the record.
            // Don't report a corruption.
            return kEof;
        }

        if (type == ns_log::RecordType::kZeroType && length == 0) {
            buffer_.clear();
            return kBadRecord;
        }
        // Check crc
        if (checksum_) {
            uint32_t expected_crc = ns_util::Unmask(ns_util::DecodeFixed32(header));
            uint32_t actual_crc = ns_util::Value(header + 6, 1 + length);
            if (actual_crc != expected_crc) {
                uint64_t drop_size = buffer_.size();
                buffer_.clear();
                ReportCorruption(drop_size, "checksum mismatch");
                return kBadRecord;
            }
        }

        buffer_.remove_prefix(ns_log::kHeaderSize + length);
        if (end_of_buffer_offset_ - buffer_.size() - ns_log::kHeaderSize - length < initial_offset_) {
            result->clear();
            return kBadRecord;
        }

        *result = ns_data_structure::Slice(header + ns_log::kHeaderSize, length);
        return type;
    }
    return UINT32_MAX;
}

void Reader::ReportCorruption(uint64_t bytes, char const *reason) {
    ReportDrop(bytes, ns_util::Status::Corruption(reason));
}

void Reader::ReportDrop(uint64_t bytes, ns_util::Status const &reason) {
    if (reporter_ != nullptr && end_of_buffer_offset_ - buffer_.size() - bytes >= initial_offset_) {
        reporter_->Corruption(static_cast<uint64_t>(bytes), reason);
    }
}

} // ns_log_reader