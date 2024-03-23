#include "write_batch.h"
#include "write_batch_internal.h"
#include "coding.h"

namespace ns_write_batch {

static constexpr uint64_t kHeader = 8 + 4;

WriteBatch::WriteBatch() {
    Clear();
}

void WriteBatch::Put(ns_data_structure::Slice const &key, ns_data_structure::Slice const &value) {
    WriteBatchInternal::SetCount(this, WriteBatchInternal::Count(this) + 1);
    rep_.push_back(static_cast<char>(ns_db_format::ValueType::kTypeValue));
    ns_util::PutLengthPrefixedSlice(&rep_, key);
    ns_util::PutLengthPrefixedSlice(&rep_, value);
}

void WriteBatch::Delete(ns_data_structure::Slice const &key) {
    WriteBatchInternal::SetCount(this, WriteBatchInternal::Count(this) + 1);
    rep_.push_back(static_cast<char>(ns_db_format::ValueType::kTypeDeletion));
    ns_util::PutLengthPrefixedSlice(&rep_, key);
}

void WriteBatch::Clear() {
    rep_.clear();
    rep_.resize(kHeader);
}

uint64_t WriteBatch::ApproximateSize() const {
    return rep_.size();
}

void WriteBatch::Append(WriteBatch const &source) {
    WriteBatchInternal::Append(this, &source);
}

ns_util::Status WriteBatch::Iterate(Handler *handler) const {
    ns_data_structure::Slice input(rep_);
    if (input.size() < kHeader) {
        return ns_util::Status::Corruption("malformed WriteBatch (too small)");
    }
    input.remove_prefix(kHeader);
    ns_data_structure::Slice key, value;
    int32_t found = 0;
    while (!input.empty()) {
        found++;
        char tag = input[0];
        input.remove_prefix(1);
        switch (tag) {
        case ns_db_format::ValueType::kTypeValue: {
            if (ns_util::GetLengthPrefixedSlice(&input, &key) && ns_util::GetLengthPrefixedSlice(&input, &value)) {
                handler->Put(key, value);
            } else {
                return ns_util::Status::Corruption("bad WriteBatch Put");
            }
            break;
        }
        case ns_db_format::ValueType::kTypeDeletion: {
            if (ns_util::GetLengthPrefixedSlice(&input, &key)) {
                handler->Delete(key);
            } else {
                return ns_util::Status::Corruption("badWriteBatch Delete");
            }
            break;
        }
        default: {
            return ns_util::Status::Corruption("unknown WriteBatch tag");
        }
        }
    }
    if (found != WriteBatchInternal::Count(this)) {
        return ns_util::Status::Corruption("WriteBatch has wrong count");
    }
    return ns_util::Status::OK();
}

int32_t WriteBatchInternal::Count(WriteBatch const *batch) {
    return ns_util::DecodeFixed32(reinterpret_cast<uint8_t const *>(batch->rep_.data() + 8));
}

void WriteBatchInternal::SetCount(WriteBatch *batch, int32_t n) {
    ns_util::EncodeFixed32(reinterpret_cast<uint8_t *>(&batch->rep_[8]), n);
}

ns_db_format::SequenceNumber WriteBatchInternal::Sequence(WriteBatch const *batch) {
    return ns_db_format::SequenceNumber(ns_util::DecodeFixed64(reinterpret_cast<uint8_t const *>(batch->rep_.data())));
}

void WriteBatchInternal::SetSequence(WriteBatch *batch, ns_db_format::SequenceNumber seq) {
    ns_util::EncodeFixed64(reinterpret_cast<uint8_t *>(&batch->rep_[0]), seq);
}

void WriteBatchInternal::SetContents(WriteBatch *batch, ns_data_structure::Slice const &contents) {
    assert(contents.size() >= kHeader);
    batch->rep_.assign(reinterpret_cast<char const *>(contents.data()), contents.size());
}

namespace {
class MemTableInserter : public WriteBatch::Handler {
public:
    ns_db_format::SequenceNumber sequence_;
    ns_data_structure::MemTable *mem_;

    void Put(ns_data_structure::Slice const &key, ns_data_structure::Slice const &value) override {
        mem_->Add(sequence_, ns_db_format::ValueType::kTypeValue, key, value);
        sequence_++;
    }

    void Delete(ns_data_structure::Slice const &key) override {
        mem_->Add(sequence_, ns_db_format::ValueType::kTypeDeletion, key, ns_data_structure::Slice());
        sequence_++;
    }
};
} // namespace

ns_util::Status WriteBatchInternal::InsertInto(WriteBatch const *batch, ns_data_structure::MemTable *memtable) {
    MemTableInserter inserter;
    inserter.sequence_ = WriteBatchInternal::Sequence(batch);
    inserter.mem_ = memtable;
    return batch->Iterate(&inserter);
}

void WriteBatchInternal::Append(WriteBatch *dst, WriteBatch const *src) {
    SetCount(dst, Count(dst) + Count(src));
    assert(src->rep_.size() >= kHeader);
    dst->rep_.append(src->rep_.data() + kHeader, src->rep_.size() - kHeader);
}
} // ns_write_batch