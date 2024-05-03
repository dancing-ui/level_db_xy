#include "block.h"
#include "coding.h"
#include "iterator.h"
#include "status.h"

namespace ns_table {

uint32_t Block::NumRestarts() const {
    assert(size_ >= sizeof(uint32_t));
    return ns_util::DecodeFixed32(data_ + size_ - sizeof(uint32_t));
}

Block::Block(BlockContents const &contents) :
    data_(contents.data.data()),
    size_(contents.data.size()),
    owned_(contents.heap_allocated) {
    if (size_ < sizeof(uint32_t)) {
        size_ = 0; // Error marker
    } else {
        uint64_t max_restarts_allowed = (size_ - sizeof(uint32_t)) / sizeof(uint32_t);
        if (NumRestarts() > max_restarts_allowed) {
            // The size is too small for NumRestarts()
            size_ = 0;
        } else {
            restart_offset_ = size_ + (1 + NumRestarts()) * sizeof(uint32_t);
        }
    }
}

Block::~Block() {
    if (owned_) {
        delete[] data_;
    }
}
// Helper routine: decode the next block entry starting at "p",
// storing the number of shared key bytes, non_shared key bytes,
// and the length of the value in "*shared", "*non_shared", and
// "*value_length", respectively.  Will not dereference past "limit".
//
// If any errors are detected, returns nullptr.  Otherwise, returns a
// pointer to the key delta (just past the three decoded values).
static inline uint8_t const *DecodeEntry(uint8_t const *p, uint8_t const *limit,
                                         uint32_t *shared, uint32_t *non_shared,
                                         uint32_t *value_length) {
    if (limit - p < 3) {
        return nullptr;
    }
    *shared = p[0];
    *non_shared = p[1];
    *value_length = p[2];
    if ((*shared | *non_shared | *value_length) < 128) {
        // Fast path: all three values are encoded in one byte each
        p += 3;
    } else {
        if ((p = ns_util::GetVarint32Ptr(p, limit, shared)) == nullptr) {
            return nullptr;
        }
        if ((p = ns_util::GetVarint32Ptr(p, limit, non_shared)) == nullptr) {
            return nullptr;
        }
        if ((p = ns_util::GetVarint32Ptr(p, limit, value_length)) == nullptr) {
            return nullptr;
        }
    }

    if (static_cast<uint32_t>(limit - p) < (*non_shared + *value_length)) {
        return nullptr;
    }
    return p;
}

class Block::Iter : public ns_iterator::Iterator {
private:
    ns_comparator::Comparator const *const comparator_;
    uint8_t const *const data_;   // underlying block contents
    uint32_t const restarts_;     // Offset of restart array (list of fixed32)
    uint32_t const num_restarts_; // Number of uint32_t entries in restart array

    uint32_t current_;       // current_ is offset in data_ of current entry.  >= restarts_ if !Valid
    uint32_t restart_index_; // Index of restart block in which current_ falls
    std::string key_;
    ns_data_structure::Slice value_;
    ns_util::Status status_;

    inline int32_t Compare(ns_data_structure::Slice const &a, ns_data_structure::Slice const &b) const {
        return comparator_->Compare(a, b);
    }

    inline uint32_t NextEntryOffset() const {
        return (value_.data() + value_.size()) - data_;
    }

    uint32_t GetRestartPoint(uint32_t index) {
        assert(index < num_restarts_);
        return ns_util::DecodeFixed32(data_ + restarts_ + index * sizeof(uint32_t));
    }

    void SeekToRestartPoint(uint32_t index) {
        key_.clear();
        restart_index_ = index;
        // current_ will be fixed by ParseNextKey();

        // ParseNextKey() starts at the end of value_, so set value_ accordingly
        uint32_t offset = GetRestartPoint(index);
        value_ = ns_data_structure::Slice(data_ + offset, 0);
    }

public:
    Iter(ns_comparator::Comparator const *comparator, uint8_t const *data,
         uint32_t restarts, uint32_t num_restarts) :
        comparator_(comparator),
        data_(data),
        restarts_(restarts),
        num_restarts_(num_restarts),
        current_(restarts),
        restart_index_(num_restarts) {
        assert(num_restarts_ > 0);
    }

    bool Valid() const override {
        return current_ < restarts_;
    }

    ns_util::Status status() const override {
        return status_;
    }

    ns_data_structure::Slice key() const override {
        assert(Valid());
        return key_;
    }

    ns_data_structure::Slice value() const override {
        assert(Valid());
        return value_;
    }

    void Next() override {
        assert(Valid());
        ParseNextKey();
    }

    void Prev() override {
        assert(Valid());

        uint32_t const original{current_};
        while (GetRestartPoint(restart_index_) >= original) {
            if (restart_index_ == 0) {
                current_ = restarts_;
                restart_index_ = num_restarts_;
                return;
            }
            restart_index_--;
        }

        SeekToRestartPoint(restart_index_);
        do {
            // Loop until end of current entry hits the start of original entry
        } while (ParseNextKey() && NextEntryOffset() < original);
    }

    void Seek(ns_data_structure::Slice const &target) override {
        // Binary search in restart array to find the last restart point
        // with a key < target
        uint32_t left{0UL};
        uint32_t right{num_restarts_ - 1};
        int32_t current_key_compare{0};

        if (Valid()) {
            // If we're already scanning, use the current position as a starting
            // point. This is beneficial if the key we're seeking to is ahead of the
            // current position.
            current_key_compare = Compare(key_, target);
            if (current_key_compare < 0) {
                // key_ is smaller than target
                left = restart_index_;
            } else if (current_key_compare > 0) {
                right = restart_index_;
            } else {
                // We're seeking to the key we're already at.
                return;
            }
        }

        while (left < right) {
            uint32_t mid = (left + right + 1) / 2;
            uint32_t region_offset = GetRestartPoint(mid);
            uint32_t shared, non_shared, value_length;
            uint8_t const *key_ptr = DecodeEntry(data_ + region_offset, data_ + restarts_, &shared, &non_shared, &value_length);
            if (key_ptr == nullptr || (shared != 0)) {
                CorruptionError();
                return;
            }
            ns_data_structure::Slice mid_key{key_ptr, non_shared};
            if (Compare(mid_key, target) < 0) {
                left = mid;
            } else {
                right = mid - 1;
            }
        }
        // We might be able to use our current position within the restart block.
        // This is true if we determined the key we desire is in the current block
        // and is after than the current key.
        assert(current_key_compare == 0 || Valid());
        bool skip_seek = (left == restart_index_ && current_key_compare < 0);
        if (!skip_seek) {
            SeekToRestartPoint(left);
        }
        // Linear search (within restart block) for first key >= target
        while (true) {
            if (!ParseNextKey()) {
                return;
            }
            if (Compare(key_, target) >= 0) {
                return;
            }
        }
    }

    void SeekToFirst() override {
        SeekToRestartPoint(0);
        ParseNextKey();
    }

    void SeekToLast() override {
        SeekToRestartPoint(num_restarts_ - 1);
        while (ParseNextKey() && NextEntryOffset() < restarts_) {
            // Keep skipping
        }
    }

private:
    void CorruptionError() {
        current_ = restarts_;
        restart_index_ = num_restarts_;
        status_ = ns_util::Status::Corruption("bad entry in block");
        key_.clear();
        value_.clear();
    }

    bool ParseNextKey() {
        current_ = NextEntryOffset();
        uint8_t const *p = data_ + current_;
        uint8_t const *limit = data_ + restarts_; // restarts come right after data
        if (p >= limit) {
            // No more entries to return.  Mark as invalid.
            current_ = restarts_;
            restart_index_ = num_restarts_;
            return false;
        }

        uint32_t shared, non_shared, value_length;
        p = DecodeEntry(p, limit, &shared, &non_shared, &value_length);
        if (p == nullptr || key_.size() < shared) {
            CorruptionError();
            return false;
        }
        key_.resize(shared);
        key_.append(reinterpret_cast<char const *>(p), non_shared);
        value_ = ns_data_structure::Slice(p + non_shared, value_length);
        while (restart_index_ + 1 < num_restarts_ && GetRestartPoint(restart_index_ + 1) < current_) {
            ++restart_index_;
        }
        return true;
    }
};

ns_iterator::Iterator *Block::NewIterator(ns_comparator::Comparator const *comparator) {
    if (size_ < sizeof(uint32_t)) {
        return ns_iterator::NewErrorIterator(ns_util::Status::Corruption("bad block contents"));
    }
    uint32_t const num_restarts = NumRestarts();
    if (num_restarts == 0) {
        return ns_iterator::NewEmptyIterator();
    }
    return new Iter(comparator, data_, restart_offset_, num_restarts);
}

} // ns_table