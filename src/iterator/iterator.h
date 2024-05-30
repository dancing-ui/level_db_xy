#ifndef _LEVEL_DB_XY_ITERATOR_H_
#define _LEVEL_DB_XY_ITERATOR_H_

#include "status.h"
#include "slice.h"

#include <functional>

namespace ns_iterator {

class Iterator {
public:
    Iterator();
    Iterator(Iterator const &) = delete;
    Iterator &operator=(Iterator const &) = delete;

    virtual ~Iterator();
    virtual bool Valid() const = 0;
    virtual void SeekToFirst() = 0;
    virtual void SeekToLast() = 0;
    virtual void Seek(ns_data_structure::Slice const &target) = 0;
    virtual void Next() = 0;
    virtual void Prev() = 0;
    virtual ns_data_structure::Slice key() const = 0;
    virtual ns_data_structure::Slice value() const = 0;
    virtual ns_util::Status status() const = 0;

    using CleanupFunction = std::function<void(void *arg1, void *arg2)>;
    void RegisterCleanup(CleanupFunction const &function, void *arg1, void *arg2);

private:
    struct CleanupNode {
        bool IsEmpty() const {
            return function == nullptr;
        }
        void Run() {
            assert(function != nullptr);
            function(arg1, arg2);
        }
        CleanupFunction function;
        void *arg1;
        void *arg2;
        CleanupNode *next;
    };
    CleanupNode cleanup_head_;
};

Iterator *NewEmptyIterator();
Iterator *NewErrorIterator(ns_util::Status const &status);

} // ns_iterator

#endif