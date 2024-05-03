#include "block_builder.h"

namespace ns_table {

BlockBuilder::BlockBuilder(ns_options::Options const *options) :
    options_(options), restarts_(), counter_(0), finished_(false) {
    assert(options->block_restart_interval >= 1);
    restarts_.push_back(0); // First restart point is at offset 0
}

void BlockBuilder::Reset() {
    buffer_.clear();
    restarts_.clear();
    restarts_.push_back(0); // First restart point is at offset 0
    counter_ = 0;
    finished_ = false;
    last_key_.clear();
}

void BlockBuilder::Add(ns_data_structure::Slice const &key, ns_data_structure::Slice const &value) {
    ns_data_structure::Slice last_key_piece{last_key_};
    assert(!finished_);
    assert(counter_ <= options_->block_restart_interval);
    assert(buffer_.empty() || options_->comparator->Compare(key, last_key_piece) > 0);
    uint64_t shared{0};
    if (counter_ < options_->block_restart_interval) {
        uint64_t const min_length{std::min(last_key_piece.size(), key.size())};
        while ((shared < min_length) && (last_key_piece[shared] == key[shared])) {
            shared++;
        }
    } else {
        // Restart compression
        restarts_.push_back(buffer_.size());
        counter_ = 0;
    }

    uint64_t const non_shared{key.size() - shared};
    // Add "<shared><non_shared><value_size>" to buffer_
    ns_util::PutVarint32(&buffer_, shared);
    ns_util::PutVarint32(&buffer_, non_shared);
    ns_util::PutVarint32(&buffer_, value.size());

    // Add string delta to buffer_ followed by value
    buffer_.append(reinterpret_cast<char const *>(key.data() + shared), non_shared);
    buffer_.append(reinterpret_cast<char const *>(value.data()), value.size());

    // Update state
    last_key_.resize(shared);
    last_key_.append(reinterpret_cast<char const *>(key.data() + shared), non_shared);
    assert(ns_data_structure::Slice(last_key_) == key);
    counter_++;
}

ns_data_structure::Slice BlockBuilder::Finish() {
    for (uint64_t i = 0; i < restarts_.size(); i++) {
        ns_util::PutFixed32(&buffer_, restarts_[i]);
    }
    ns_util::PutFixed32(&buffer_, restarts_.size());
    finished_ = true;
    return ns_data_structure::Slice(buffer_);
}

uint64_t BlockBuilder::CurrentSizeEstimate() const {
    // Raw data buffer + Restart array + Restart array length
    return (buffer_.size() + restarts_.size() * sizeof(uint32_t) + sizeof(uint32_t));
}

}