#include "db_format.h"
#include "logging.h"
#include "coding.h"
#include <sstream>

namespace ns_db_format {

uint64_t PackSequenceAndType(uint64_t seq, ValueType const &t) {
    assert(seq <= kMaxSequenceNumber);
    assert(t <= kValueTypeForSeek);
    return (seq << 8) | t;
}

void AppendInternalKey(std::string *result, ParsedInternalKey const &key) {
    result->append(reinterpret_cast<char const *>(key.user_key.data()));
    ns_util::PutFixed64(result, PackSequenceAndType(key.sequence, key.type));
}

std::string ParsedInternalKey::DebugString() const {
    std::ostringstream ss;
    ss << '\'' << ns_log::EscapeString(user_key.ToString()) << "' @ " << sequence << " : "
       << static_cast<int32_t>(type);
    return ss.str();
}

std::string InternalKey::DebugString() const {
    ParsedInternalKey parsed;
    if (ParseInternalKey(rep_, &parsed)) {
        return parsed.DebugString();
    }
    std::ostringstream ss;
    ss << "(bad)" << ns_log::EscapeString(rep_);
    return ss.str();
}

int32_t InternalKeyComparator::Compare(ns_data_structure::Slice const &a_key, ns_data_structure::Slice const &b_key) const {
    int32_t ret = user_comparator_->Compare(ExtractUserKey(a_key), ExtractUserKey(b_key));
    if (ret == 0) {
        uint64_t const anum = ns_util::DecodeFixed64(a_key.data() + a_key.size() - 8);
        uint64_t const bnum = ns_util::DecodeFixed64(b_key.data() + b_key.size() - 8);
        if (anum > bnum) {
            ret = -1;
        } else {
            ret = 1;
        }
    }
    return ret;
}
uint8_t const *InternalKeyComparator::Name() const {
    return reinterpret_cast<uint8_t const *>("leveldb.InternalKeyComparator");
}
void InternalKeyComparator::FindShortestSeparator(std::string *start, ns_data_structure::Slice const &limit) const {
    ns_data_structure::Slice user_start = ExtractUserKey(*start);
    ns_data_structure::Slice user_limit = ExtractUserKey(limit);
    std::string tmp(reinterpret_cast<char const *>(user_start.data()), user_start.size());
    user_comparator_->FindShortestSeparator(&tmp, user_limit);
    if (tmp.size() < user_start.size() && user_comparator_->Compare(user_start, tmp) < 0) {
        ns_util::PutFixed64(&tmp, PackSequenceAndType(kMaxSequenceNumber, kValueTypeForSeek));
        assert(this->Compare(*start, tmp) < 0);
        assert(this->Compare(tmp, limit) < 0);
        start->swap(tmp);
    }
}
void InternalKeyComparator::FindShortSuccessor(std::string *key) const {
    ns_data_structure::Slice user_key = ExtractUserKey(*key);
    std::string tmp(reinterpret_cast<char const *>(user_key.data()), user_key.size());
    user_comparator_->FindShortSuccessor(&tmp);
    if (tmp.size() < user_key.size() && user_comparator_->Compare(user_key, tmp) < 0) {
        ns_util::PutFixed64(&tmp, PackSequenceAndType(kMaxSequenceNumber, kValueTypeForSeek));
        assert(this->Compare(*key, tmp) < 0);
        key->swap(tmp);
    }
}

LookupKey::LookupKey(ns_data_structure::Slice const &user_key, SequenceNumber sequence) {
    uint64_t usize = user_key.size();
    uint64_t needed = usize + 13; // 13 is a conservative estimate
    uint8_t *dst;
    if (needed <= sizeof(space_)) {
        dst = space_;
    } else {
        dst = new uint8_t[needed];
    }
    start_ = dst;
    dst = ns_util::EncodeVarint32(dst, usize + 8);
    kstart_ = dst;
    std::memcpy(dst, user_key.data(), usize);
    dst += usize;
    ns_util::EncodeFixed64(dst, PackSequenceAndType(sequence, kValueTypeForSeek));
    dst += 8;
    end_ = dst;
}

} // ns_db_format