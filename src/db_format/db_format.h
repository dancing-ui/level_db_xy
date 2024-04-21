#ifndef _LEVEL_DB_XY_DB_FORMAT_H_
#define _LEVEL_DB_XY_DB_FORMAT_H_

#include "slice.h"
#include "comparator.h"
#include "coding.h"
#include <cassert>

namespace ns_db_format {

using SequenceNumber = uint64_t;

enum ValueType {
    kTypeDeletion = 0x0,
    kTypeValue = 0x1
};
static constexpr SequenceNumber kMaxSequenceNumber{(0x1ULL << 56) - 1};
static constexpr ValueType kValueTypeForSeek{kTypeValue};

struct ParsedInternalKey {
    ns_data_structure::Slice user_key;
    SequenceNumber sequence;
    ValueType type;

    ParsedInternalKey() = default;
    ParsedInternalKey(ns_data_structure::Slice const &u, SequenceNumber const &seq, ValueType const &t) :
        user_key(u), sequence(seq), type(t) {
    }
    std::string DebugString() const;
};

void AppendInternalKey(std::string *result, ParsedInternalKey const &key);

inline ns_data_structure::Slice ExtractUserKey(ns_data_structure::Slice const &internal_key) {
    assert(internal_key.size() >= 8);
    return ns_data_structure::Slice(internal_key.data(), internal_key.size() - 8);
}

inline bool ParseInternalKey(ns_data_structure::Slice const &internal_key, ParsedInternalKey *result) {
    uint64_t const n = internal_key.size();
    if (n < 8) {
        return false;
    }
    uint64_t num = ns_util::DecodeFixed64(internal_key.data() + n - 8);
    uint8_t c = num & 0xFFU;
    result->sequence = num >> 8;
    result->type = static_cast<ValueType>(c);
    result->user_key = ns_data_structure::Slice(internal_key.data(), n - 8);
    return c <= static_cast<uint8_t>(kTypeValue);
}

class InternalKey {
private:
    std::string rep_;

public:
    InternalKey() = default;
    InternalKey(ns_data_structure::Slice const &user_key, SequenceNumber s, ValueType t) {
        AppendInternalKey(&rep_, ParsedInternalKey(user_key, s, t));
    }

    bool DecodeFrom(ns_data_structure::Slice const &s) {
        rep_.assign(reinterpret_cast<char const *>(s.data()), s.size());
        return !rep_.empty();
    }

    ns_data_structure::Slice Encode() const {
        assert(!rep_.empty());
        return rep_;
    }

    ns_data_structure::Slice user_key() const {
        return ExtractUserKey(rep_);
    }

    void SetFrom(ParsedInternalKey const &p) {
        rep_.clear();
        AppendInternalKey(&rep_, p);
    }

    void Clear() {
        rep_.clear();
    }

    std::string DebugString() const;
};

uint64_t PackSequenceAndType(uint64_t seq, ValueType const &t);

class InternalKeyComparator : public ns_comparator::Comparator {
private:
    Comparator const *user_comparator_;

public:
    explicit InternalKeyComparator(Comparator const *c) :
        user_comparator_(c) {
    }
    int32_t Compare(ns_data_structure::Slice const &a_key, ns_data_structure::Slice const &b_key) const override;
    uint8_t const *Name() const override;
    void FindShortestSeparator(std::string *start, ns_data_structure::Slice const &limit) const override;
    void FindShortSuccessor(std::string *key) const override;

    Comparator const *user_comparator() const {
        return user_comparator_;
    }
    int32_t Compare(InternalKey const &a, InternalKey const &b) const {
        return Compare(a.Encode(), b.Encode());
    }
};

class LookupKey {
public:
    LookupKey(ns_data_structure::Slice const &user_key, SequenceNumber sequence);
    LookupKey(LookupKey const &) = delete;
    LookupKey &operator=(LookupKey const &) = delete;
    ~LookupKey() {
        if (start_ != space_) {
            delete[] start_;
        }
    }
    ns_data_structure::Slice memtable_key() const {
        return ns_data_structure::Slice(start_, end_ - start_);
    }
    ns_data_structure::Slice internal_key() const {
        return ns_data_structure::Slice(kstart_, end_ - kstart_);
    }
    ns_data_structure::Slice user_key() const {
        return ns_data_structure::Slice(kstart_, end_ - kstart_ - 8);
    }

private:
    // We construct a char array of the form:
    //    klength  varint32               <-- start_
    //    userkey  char[klength]          <-- kstart_
    //    tag      uint64
    //                                    <-- end_
    uint8_t const *start_;
    uint8_t const *kstart_;
    uint8_t const *end_;
    uint8_t space_[200];
};

} // ns_db_format

#endif