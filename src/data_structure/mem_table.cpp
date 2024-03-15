#include "mem_table.h"
#include "coding.h"

namespace ns_data_structure {

static ns_data_structure::Slice GetLengthPrefixedSlice(uint8_t const* data) {
    uint32_t len;
    uint8_t const* p = data;
    p = ns_util::GetVarint32Ptr(p, p + 5, &len);
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

ns_iterator::Iterator *MemTable::NewIterator() {

}

void MemTable::Add(ns_db_format::SequenceNumber seq, ns_db_format::ValueType type, ns_data_structure::Slice const &key, ns_data_structure::Slice const &value) {
}

bool MemTable::Get(ns_db_format::LookupKey const &key, std::string *value, ns_util::Status *s) {
}

int32_t MemTable::KeyComparator::operator()(uint8_t const *ap, uint8_t const *bp) const {
    ns_data_structure::Slice a = GetLengthPrefixedSlice(ap);
    ns_data_structure::Slice b = GetLengthPrefixedSlice(bp);
    return comparator.Compare(a, b);
}

} // ns_data_structure