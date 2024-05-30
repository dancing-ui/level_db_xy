#ifndef _LEVEL_DB_XY_ITERATOR_WRAPPER_H_
#define _LEVEL_DB_XY_ITERATOR_WRAPPER_H_

#include "iterator.h"
#include "status.h"

namespace ns_table {

// A internal wrapper class with an interface similar to Iterator that
// caches the valid() and key() results for an underlying iterator.
// This can help avoid virtual function calls and also gives better
// cache locality.
class IteratorWrapper {
public:
    IteratorWrapper() :
        iter_(nullptr), valid_(false) {
    }
    explicit IteratorWrapper(ns_iterator::Iterator *iter) :
        iter_(nullptr) {
        Set(iter);
    }
    ~IteratorWrapper() {
        delete iter_;
    }
    ns_iterator::Iterator *iter() const {
        return iter_;
    }

    void Set(ns_iterator::Iterator *iter) {
        delete iter_;
        iter_ = iter;
        if (iter_ == nullptr) {
            valid_ = false;
        } else {
            Update();
        }
    }

    bool Valid() const {
        return valid_;
    }

    ns_data_structure::Slice key() const {
        assert(Valid());
        return key_;
    }

    ns_data_structure::Slice value() const {
        assert(Valid());
        return iter_->value();
    }

    ns_util::Status status() const {
        assert(iter_);
        return iter_->status();
    }

    void Next() {
        assert(iter_);
        iter_->Next();
        Update();
    }

    void Prev() {
        assert(iter_);
        iter_->Prev();
        Update();
    }

    void Seek(ns_data_structure::Slice const &k) {
        assert(iter_);
        iter_->Seek(k);
        Update();
    }

    void SeekToFirst() {
        assert(iter_);
        iter_->SeekToFirst();
        Update();
    }

    void SeekToLast() {
        assert(iter_);
        iter_->SeekToLast();
        Update();
    }

private:
    void Update() {
        valid_ = iter_->Valid();
        if (valid_) {
            key_ = iter_->key();
        }
    }

    ns_iterator::Iterator *iter_;
    bool valid_;
    ns_data_structure::Slice key_;
};

} // ns_table

#endif