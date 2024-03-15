#include "iterator.h"

namespace ns_iterator {

Iterator::Iterator() {
    cleanup_head_.function = nullptr;
    cleanup_head_.next = nullptr;
}
Iterator::~Iterator() {
    if (!cleanup_head_.IsEmpty()) {
        cleanup_head_.Run();
        for (CleanupNode *node = cleanup_head_.next; node != nullptr;) {
            node->Run();
            CleanupNode *next_node = node->next;
            delete node;
            node = next_node;
        }
    }
}
void Iterator::RegisterCleanup(CleanupFunction function, void *arg1, void *arg2) {
    assert(function != nullptr);
    CleanupNode *node;
    if (cleanup_head_.IsEmpty()) {
        node = &cleanup_head_;
    } else {
        node = new CleanupNode();
        node->next = cleanup_head_.next;
        cleanup_head_.next = node;
    }
    node->function = function;
    node->arg1 = arg1;
    node->arg2 = arg2;
}

class EmptyIterator : public Iterator {
public:
    EmptyIterator(ns_util::Status const &s) :
        status_(s) {
    }
    ~EmptyIterator() override = default;

    bool Valid() const override {
        return false;
    }
    void SeekToFirst() override {
    }
    void SeekToLast() override {
    }
    void Seek(ns_data_structure::Slice const &target) override {
    }
    void Next() override {
        assert(false);
    }
    void Prev() override {
        assert(false);
    }
    ns_data_structure::Slice key() const {
        assert(false);
        return ns_data_structure::Slice();
    }
    ns_data_structure::Slice value() const {
        assert(false);
        return ns_data_structure::Slice();
    }
    ns_util::Status status() const {
        return status_;
    }

private:
    ns_util::Status status_;
};

Iterator *NewEmptyIterator() {
    return new EmptyIterator(ns_util::Status::OK());
}
Iterator *NewErrorIterator(ns_util::Status const &status) {
    return new EmptyIterator(status);
}

} // ns_iterator
