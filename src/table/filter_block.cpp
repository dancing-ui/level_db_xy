#include "filter_block.h"
#include "coding.h"

namespace ns_table {

static constexpr uint64_t kFilterBaseLg = 11;
static constexpr uint64_t kFilterBase = 1 << kFilterBaseLg;

FilterBlockBuilder::FilterBlockBuilder(ns_filter_policy::FilterPolicy const *policy) :
    policy_(policy) {
}

void FilterBlockBuilder::StartBlock(uint64_t block_offset) {
    uint64_t filter_index = (block_offset / kFilterBase);
    assert(filter_index >= filter_offsets_.size());
    while (filter_index > filter_offsets_.size()) {
        GenerateFilter();
    }
}

void FilterBlockBuilder::AddKey(ns_data_structure::Slice const &key) {
    ns_data_structure::Slice k = key;
    start_.push_back(keys_.size());
    keys_.append(reinterpret_cast<char const *>(k.data()), k.size());
}

ns_data_structure::Slice FilterBlockBuilder::Finish() {
    if (!start_.empty()) {
        GenerateFilter();
    }

    uint32_t const array_offset = result_.size();
    for (uint64_t i = 0; i < filter_offsets_.size(); i++) {
        ns_util::PutFixed32(&result_, filter_offsets_[i]);
    }

    ns_util::PutFixed32(&result_, array_offset);
    result_.push_back(kFilterBaseLg); // Save encoding parameter in result
    return ns_data_structure::Slice(result_);
}

void FilterBlockBuilder::GenerateFilter() {
    uint64_t const num_keys = start_.size();
    if (num_keys == 0) {
        // Fast path if there are no keys for this filter
        filter_offsets_.push_back(result_.size());
        return;
    }

    start_.push_back(keys_.size()); // Simplify length computation
    tmp_keys_.resize(num_keys);
    for (uint64_t i = 0; i < num_keys; i++) {
        char const *base = keys_.data() + start_[i];
        uint64_t length = start_[i + 1] - start_[i];
        tmp_keys_[i] = ns_data_structure::Slice(base, length);
    }
    // Generate filter for current set of keys and append to result_.
    filter_offsets_.push_back(result_.size());
    policy_->CreateFilter(&tmp_keys_[0], static_cast<int32_t>(num_keys), &result_);

    tmp_keys_.clear();
    keys_.clear();
    start_.clear();
}

FilterBlockReader::FilterBlockReader(ns_filter_policy::FilterPolicy const *policy, ns_data_structure::Slice const &contents) :
    policy_(policy), data_(nullptr),
    offset_(nullptr), num_(0), base_lg_(0) {
    uint64_t n = contents.size();
    if (n < 5) {
        return; // 1 byte for base_lg_ and 4 for start of offset array
    }
    base_lg_ = contents[n - 1];
    uint32_t last_word = ns_util::DecodeFixed32(contents.data() + n - 5);
    if (last_word > n - 5) {
        return;
    }
    data_ = contents.data();
    offset_ = data_ + last_word;
    num_ = (n - 5 - last_word) / 4;
}

bool FilterBlockReader::KeyMayMatch(uint64_t block_offset, ns_data_structure::Slice const &key) {
    uint64_t index = block_offset >> base_lg_;
    if (index < num_) {
        uint32_t start = ns_util::DecodeFixed32(offset_ + index * 4);
        uint32_t limit = ns_util::DecodeFixed32(offset_ + index * 4 + 4);
        if (start <= limit && limit <= static_cast<uint64_t>(offset_ - data_)) {
            ns_data_structure::Slice filter = ns_data_structure::Slice(data_ + start, limit - start);
            return policy_->KeyMayMatch(key, filter);
        } else if (start == limit) {
            // Empty filters do not match any keys
            return false;
        }
    }
    return true; // Errors are treated as potential matches
}

} // ns_table