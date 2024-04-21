#include "mem_table.h"
#include "coding.h"

namespace ns_data_structure {

static ns_data_structure::Slice GetLengthPrefixedSlice(uint8_t const *data) {
    uint32_t len;
    uint8_t const *p = data;
    p = ns_util::GetVarint32Ptr(p, p + 5, &len); // 按变长方式存储32位正整数最多需要5字节
    return ns_data_structure::Slice(p, len);
}

MemTable::MemTable(ns_db_format::InternalKeyComparator const &comparator) :
    comparator_(comparator),
    refs_(0),
    table_(comparator_, &arena_) {
}
uint64_t MemTable::ApproximateMemoryUsage() {
    return arena_.MemoryUsage();
}

static uint8_t const *EncodeKey(std::string *scratch, ns_data_structure::Slice const &target) {
    scratch->clear();
    ns_util::PutVarint32(scratch, target.size());
    scratch->append(reinterpret_cast<char const *>(target.data()), target.size());
    return reinterpret_cast<uint8_t const *>(scratch->data());
}

class MemTableIterator : public ns_iterator::Iterator {
public:
    explicit MemTableIterator(MemTable::Table *table) :
        iter_(table) {
    }
    MemTableIterator(MemTableIterator const &) = delete;
    MemTableIterator &operator=(MemTableIterator const &) = delete;

    ~MemTableIterator() override = default;

    bool Valid() const override {
        return iter_.Valid();
    }
    void SeekToFirst() override {
        iter_.SeekToFirst();
    }
    void SeekToLast() override {
        iter_.SeekToLast();
    }
    void Seek(ns_data_structure::Slice const &target) override {
        iter_.Seek(EncodeKey(&tmp_, target));
    }
    void Next() override {
        iter_.Next();
    }
    void Prev() override {
        iter_.Prev();
    }
    ns_data_structure::Slice key() const override {
        return GetLengthPrefixedSlice(iter_.key());
    }
    ns_data_structure::Slice value() const override {
        ns_data_structure::Slice key_slice = GetLengthPrefixedSlice(iter_.key());
        return GetLengthPrefixedSlice(key_slice.data() + key_slice.size());
    }
    ns_util::Status status() const override {
        return ns_util::Status::OK();
    }

private:
    MemTable::Table::Iterator iter_;
    std::string tmp_;
};

ns_iterator::Iterator *MemTable::NewIterator() {
    return new MemTableIterator(&table_);
}

void MemTable::Add(ns_db_format::SequenceNumber seq, ns_db_format::ValueType type, ns_data_structure::Slice const &key, ns_data_structure::Slice const &value) {
    // Format of an entry is concatenation of:
    //  key_size     : varint32 of internal_key.size()
    //  key bytes    : uint8_t[internal_key.size()]
    //  tag          : uint64((sequence << 8) | type)
    //  value_size   : varint32 of value.size()
    //  value bytes  : uint8_t[value.size()]
    uint64_t key_size = key.size();
    uint64_t val_size = value.size();
    uint64_t internal_key_size = key_size + 8;
    uint64_t const encoded_len = ns_util::VarintLength(internal_key_size) + internal_key_size + ns_util::VarintLength(val_size) + val_size;
    uint8_t *buf = arena_.Allocate(encoded_len);
    uint8_t *p = ns_util::EncodeVarint32(buf, internal_key_size);
    std::memcpy(p, key.data(), key_size);
    p += key_size;
    ns_util::EncodeFixed64(p, (seq << 8 | type));
    p += 8;
    p = ns_util::EncodeVarint32(p, val_size);
    std::memcpy(p, value.data(), val_size);
    assert(p + val_size == buf + encoded_len);
    table_.Insert(buf);
}

bool MemTable::Get(ns_db_format::LookupKey const &key, std::string *value, ns_util::Status *s) {
    ns_data_structure::Slice mem_key = key.memtable_key();
    Table::Iterator iter(&table_);
    iter.Seek(mem_key.data());
    if (iter.Valid()) {
        // entry format is:
        //    klength  varint32
        //    userkey  uint8_t[klength]
        //    tag      uint64
        //    vlength  varint32
        //    value    uint8_t[vlength]
        // Check that it belongs to same user key.  We do not check the
        // sequence number since the Seek() call above should have skipped
        // all entries with overly large sequence numbers.
        uint8_t const *entry = iter.key();
        uint32_t key_length;
        uint8_t const *key_ptr = ns_util::GetVarint32Ptr(entry, entry + 5, &key_length);
        if (comparator_.comparator.user_comparator()->Compare(ns_data_structure::Slice(key_ptr, key_length - 8), key.user_key()) == 0) {
            // Correct user key
            uint64_t const tag = ns_util::DecodeFixed64(key_ptr + key_length - 8);
            switch (static_cast<ns_db_format::ValueType>(tag & 0xFFU)) {
            case ns_db_format::ValueType::kTypeValue: {
                ns_data_structure::Slice v = GetLengthPrefixedSlice(key_ptr + key_length);
                value->assign(reinterpret_cast<char const *>(v.data()), v.size());
                return true;
            }
            case ns_db_format::ValueType::kTypeDeletion: {
                *s = ns_util::Status::NotFound(ns_data_structure::Slice());
                return true;
            }
            default:
                break;
            }
        }
    }
    return false;
}

int32_t MemTable::KeyComparator::operator()(uint8_t const *ap, uint8_t const *bp) const {
    ns_data_structure::Slice a = GetLengthPrefixedSlice(ap);
    ns_data_structure::Slice b = GetLengthPrefixedSlice(bp);
    return comparator.Compare(a, b);
}

} // ns_data_structure