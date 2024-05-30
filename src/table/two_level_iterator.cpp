#include "two_level_iterator.h"
#include "iterator_wrapper.h"

namespace ns_table {

namespace {

typedef ns_iterator::Iterator *(*BlockFunction)(void *, ns_options::ReadOptions const &, ns_data_structure::Slice const &);

class TwoLevelIterator : public ns_iterator::Iterator {
public:
    TwoLevelIterator(ns_iterator::Iterator *index_iter, BlockFunction block_function, void *arg, ns_options::ReadOptions const &options);

    ~TwoLevelIterator() override;

    void Seek(ns_data_structure::Slice const &target) override;
    void SeekToFirst() override;
    void SeekToLast() override;
    void Next() override;
    void Prev() override;

    bool Valid() const override {
        return data_iter_.Valid();
    }

    ns_data_structure::Slice key() const override {
        assert(Valid());
        return data_iter_.value();
    }

    ns_data_structure::Slice value() const override {
        assert(Valid());
        return data_iter_.value();
    }

    ns_util::Status status() const override {
        if (!index_iter_.status().ok()) {
            return index_iter_.status();
        } else if (data_iter_.iter() != nullptr && !data_iter_.status().ok()) {
            return data_iter_.status();
        }
        return status_;
    }

private:
    void SaveError(ns_util::Status const &s) {
        if (status_.ok() && !s.ok()) {
            status_ = s;
        }
    }

    void SkipEmptyDataBlockForward();
    void SkipEmptyDataBlockBackward();
    void SetDataIterator(ns_iterator::Iterator *data_iter);
    void InitDataBlock();

    BlockFunction block_function_;
    void *arg_;
    ns_options::ReadOptions const options_;
    ns_util::Status status_;
    IteratorWrapper index_iter_;
    IteratorWrapper data_iter_; // May be nullptr
    // If data_iter_ is non-null, then "data_block_handle_" holds the
    // "index_value" passed to block_function_ to create the data_iter_.
    std::string data_block_handle_;
};

TwoLevelIterator::TwoLevelIterator(ns_iterator::Iterator *index_iter, BlockFunction block_function, void *arg, ns_options::ReadOptions const &options) :
    block_function_(block_function), arg_(arg),
    options_(options), index_iter_(index_iter),
    data_iter_(nullptr) {
}

TwoLevelIterator::~TwoLevelIterator() = default;

void TwoLevelIterator::Seek(ns_data_structure::Slice const &target) {
    index_iter_.Seek(target);
    InitDataBlock();
    if (data_iter_.iter() != nullptr) {
        data_iter_.Seek(target);
    }
    SkipEmptyDataBlockForward();
}

void TwoLevelIterator::SeekToFirst() {
    index_iter_.SeekToFirst();
    InitDataBlock();
    if (data_iter_.iter() != nullptr) {
        data_iter_.SeekToFirst();
    }
    SkipEmptyDataBlockForward();
}

void TwoLevelIterator::SeekToLast() {
    index_iter_.SeekToLast();
    InitDataBlock();
    if (data_iter_.iter() != nullptr) {
        data_iter_.SeekToLast();
    }
    SkipEmptyDataBlockBackward();
}

void TwoLevelIterator::Next() {
    assert(Valid());
    data_iter_.Next();
    SkipEmptyDataBlockForward();
}

void TwoLevelIterator::Prev() {
    assert(Valid());
    data_iter_.Prev();
    SkipEmptyDataBlockBackward();
}

void TwoLevelIterator::SkipEmptyDataBlockForward() {
    while (data_iter_.iter() == nullptr || !data_iter_.Valid()) {
        // Move to next block
        if (!index_iter_.Valid()) {
            SetDataIterator(nullptr);
            return;
        }
        index_iter_.Next();
        InitDataBlock();
        if (data_iter_.iter() != nullptr) {
            data_iter_.SeekToFirst();
        }
    }
}

void TwoLevelIterator::SkipEmptyDataBlockBackward() {
    while (data_iter_.iter() == nullptr || !data_iter_.Valid()) {
        // Move to next block
        if (!index_iter_.Valid()) {
            SetDataIterator(nullptr);
            return;
        }
        index_iter_.Prev();
        InitDataBlock();
        if (data_iter_.iter() != nullptr) {
            data_iter_.SeekToLast();
        }
    }
}

void TwoLevelIterator::SetDataIterator(ns_iterator::Iterator *data_iter) {
    if (data_iter_.iter() != nullptr) {
        SaveError(data_iter_.status());
    }
    data_iter_.Set(data_iter);
}

void TwoLevelIterator::InitDataBlock() {
    if (!index_iter_.Valid()) {
        SetDataIterator(nullptr);
    } else {
        ns_data_structure::Slice handle = index_iter_.value();
        if (data_iter_.iter() != nullptr && handle.compare(data_block_handle_) == 0) {
            // data_iter_ is already constructed with this iterator, so
            // no need to change anything
        } else {
            ns_iterator::Iterator *iter = (*block_function_)(arg_, options_, handle);
            data_block_handle_.assign(reinterpret_cast<char const *>(handle.data()), handle.size());
            SetDataIterator(iter);
        }
    }
}

} // anonymous namespace

ns_iterator::Iterator *NewTwoLevelIterator(ns_iterator::Iterator *index_iter,
                                           ns_iterator::Iterator *(*block_function)(void *arg, ns_options::ReadOptions const &options, ns_data_structure::Slice const &index_value),
                                           void *arg,
                                           ns_options::ReadOptions const &options) {
    return new TwoLevelIterator(index_iter, block_function, arg, options);
}

} // ns_table