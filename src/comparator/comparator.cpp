#include "comparator.h"
#include "slice.h"
#include "no_destructor.h"

namespace ns_comparator {

class BytewiseComparatorImpl : public Comparator {
public:
    BytewiseComparatorImpl() = default;

    uint8_t const *Name() const override {
        return reinterpret_cast<uint8_t const *>("leveldb.BytewiseComparator");
    }

    int32_t Compare(ns_data_structure::Slice const &a, ns_data_structure::Slice const &b) const override {
        return a.compare(b);
    }

    void FindShortestSeparator(std::string *start, ns_data_structure::Slice const &limit) const override {
        uint64_t min_length = std::min(start->size(), limit.size());
        uint64_t diff_index = 0;
        while ((diff_index < min_length) && ((*start)[diff_index] == limit[diff_index])) {
            diff_index++;
        }
        if (diff_index >= min_length) {
            // Do not shorten if one string is a prefix of the other
        } else {
            uint8_t diff_byte = static_cast<uint8_t>((*start)[diff_index]);
            if (diff_byte < 0xFFU && diff_byte + 1 < limit[diff_index]) {
                (*start)[diff_index]++;
                start->resize(diff_index + 1);
                assert(Compare(*start, limit) < 0);
            }
        }
    }

    void FindShortSuccessor(std::string *key) const override {
        // Find first character that can be incremented
        uint64_t n = key->size();
        for (uint64_t i = 0U; i < n; i++) {
            uint8_t const byte = (*key)[i];
            if (byte != 0xFFU) {
                (*key)[i] = byte + 1;
                key->resize(i + 1);
                return;
            }
        }
    }
};

Comparator const* BytewiseComparator() {
    static ns_no_destructor::NoDestructor<BytewiseComparatorImpl> singleton;
    return singleton.get();
}

} // ns_comparator