#ifndef _LEVEL_DB_XY_SKIP_LIST_H_
#define _LEVEL_DB_XY_SKIP_LIST_H_

#include "arena.h"
#include "random.h"
#include "log.h"

#include <cassert>
#include <cstdint>

namespace ns_data_structure {

static constexpr int32_t kMaxHeight = 12;

template <typename Key, typename Comparator>
class SkipList {
private:
    struct Node;

public:
    explicit SkipList(Comparator cmp, ns_memory::Arena *arena);
    SkipList(SkipList const &) = delete;
    SkipList(SkipList &&) = delete;
    SkipList &operator=(SkipList const &) = delete;
    SkipList &operator=(SkipList &&) = delete;

    void Insert(Key const &key);
    bool Contains(Key const &key) const;

    class Iterator {
    public:
        explicit Iterator(SkipList const *list);
        bool Valid() const;
        Key const &key() const;
        void Next();
        void Prev();
        void Seek(Key const &target);
        void SeekToFirst();
        void SeekToLast();

    private:
        SkipList const *list_;
        Node *node_;
    };

private:
    int32_t GetMaxHeight() const;
    Node *NewNode(Key const &key, int32_t height);
    int32_t RandomHeight();
    bool Equal(Key const &a, Key const &b) const;
    bool KeyIsAfterNode(Key const &key, Node *n) const;
    Node *FindGreaterOrEqual(Key const &key, Node **prev) const;
    Node *FindLessThan(Key const &key) const;
    Node *FindLast() const;
    Comparator const compare_;
    ns_memory::Arena *const arena_;
    Node *const head_;
    std::atomic<int32_t> max_height_;
    ns_algorithm::Random rnd_;
};

template <typename Key, typename Comparator>
struct SkipList<Key, Comparator>::Node {
    explicit Node(Key const &k) :
        key(k) {
    }
    Key const key;

    Node *Next(int32_t n) {
        assert(n >= 0 && n < kMaxHeight);
        return next_[n].load(std::memory_order_acquire);
    }
    void SetNext(int32_t n, Node *x) {
        assert(n >= 0 && n < kMaxHeight);
        next_[n].store(x, std::memory_order_release);
    }
    Node *NoBarrier_Next(int32_t n) {
        assert(n >= 0 && n < kMaxHeight);
        return next_[n].load(std::memory_order_relaxed);
    }
    void NoBarrier_SetNext(int32_t n, Node *x) {
        assert(n >= 0 && n < kMaxHeight);
        next_[n].store(x, std::memory_order_relaxed);
    }

private:
    std::atomic<Node *> next_[1]; // 为了可以使用下标访问，如 next_[i]
};
/**
 * SkipList Impl
 */
template <typename Key, typename Comparator>
SkipList<Key, Comparator>::SkipList(Comparator cmp, ns_memory::Arena *arena) :
    compare_(cmp),
    arena_(arena),
    head_(NewNode(0, kMaxHeight)),
    max_height_(1),
    rnd_(0xDEADBEEFU) {
    for (int32_t i = 0; i < kMaxHeight; i++) {
        head_->SetNext(i, nullptr);
    }
}
template <typename Key, typename Comparator>
void SkipList<Key, Comparator>::Insert(Key const &key) {
    Node *prev[kMaxHeight];
    Node *x = FindGreaterOrEqual(key, prev);
    assert(x == nullptr || !Equal(key, x->key));
    int32_t height = RandomHeight();
    if (height > GetMaxHeight()) {
        for (int32_t i = GetMaxHeight(); i < height; i++) {
            prev[i] = head_;
        }
        max_height_.store(height, std::memory_order_relaxed);
    }
    x = NewNode(key, height);
    for (int32_t i = 0; i < height; i++) {
        x->NoBarrier_SetNext(i, prev[i]->NoBarrier_Next(i));
        prev[i]->SetNext(i, x);
    }
}
template <typename Key, typename Comparator>
bool SkipList<Key, Comparator>::Contains(Key const &key) const {
    Node *x = FindGreaterOrEqual(key, nullptr);
    if (x != nullptr && Equal(key, x->key)) {
        return true;
    }
    return false;
}
template <typename Key, typename Comparator>
int32_t SkipList<Key, Comparator>::GetMaxHeight() const {
    return max_height_.load(std::memory_order_relaxed);
}
template <typename Key, typename Comparator>
typename SkipList<Key, Comparator>::Node *SkipList<Key, Comparator>::NewNode(Key const &key, int32_t height) {
    // 总高度是height，需要height个Node指针，由于Node本身有一个Node* next_[0]， 所以需要height - 1个Node指针
    uint8_t *const node_memory = arena_->AllocateAligned(sizeof(Node) + sizeof(std::atomic<Node *>) * (height - 1));
    // 将分配的内存node_memory直接作为new的内存，不需要new再分配，且node_memory经过内存对齐，效率更高
    return new (node_memory) Node(key);
}
template <typename Key, typename Comparator>
int32_t SkipList<Key, Comparator>::RandomHeight() {
    static constexpr uint32_t kBranching{4U};
    int32_t height{1};
    while (height < kMaxHeight && rnd_.OneIn(kBranching)) {
        height++;
    }
    assert(height > 0 && height <= kMaxHeight);
    return height;
}
template <typename Key, typename Comparator>
bool SkipList<Key, Comparator>::Equal(Key const &a, Key const &b) const {
    return compare_(a, b) == 0;
}
template <typename Key, typename Comparator>
bool SkipList<Key, Comparator>::KeyIsAfterNode(Key const &key, Node *n) const {
    return (n != nullptr) && (compare_(n->key, key) < 0);
}
template <typename Key, typename Comparator>
typename SkipList<Key, Comparator>::Node *SkipList<Key, Comparator>::FindGreaterOrEqual(Key const &key, Node **prev) const {
    Node *x = head_;
    int32_t level = GetMaxHeight() - 1;
    while (true) {
        Node *next = x->Next(level);
        if(next != nullptr && compare_(next->key, key) < 0) {
          x = next;
        } else {
            if(prev != nullptr) {
                prev[level] = x;
            }
            if(level == 0) {
                return next;
            } else {
                level--;
            }
        }
    }
    return nullptr;
}
template <typename Key, typename Comparator>
typename SkipList<Key, Comparator>::Node *SkipList<Key, Comparator>::FindLessThan(Key const &key) const {
    Node *x = head_;
    int32_t level = GetMaxHeight() - 1;
    while (true) {
        assert(x == head_ || compare_(x->key, key) < 0);
        Node *next = x->Next(level);
        if (next == nullptr || compare_(next->key, key) >= 0) {
            if (level == 0) {
                return x;
            } else {
                level--;
            }
        } else {
            x = next;
        }
    }
    return nullptr;
}
template <typename Key, typename Comparator>
typename SkipList<Key, Comparator>::Node *SkipList<Key, Comparator>::FindLast() const {
    Node *x = head_;
    int32_t level = GetMaxHeight() - 1;
    while (true) {
        Node *next = x->Next(level);
        if (next == nullptr) {
            if (level == 0) {
                return x;
            } else {
                level--;
            }
        } else {
            x = next;
        }
    }
    return nullptr;
}
/**
 * SkipList::Iterator Impl
 */
template <typename Key, typename Comparator>
SkipList<Key, Comparator>::Iterator::Iterator(SkipList const *list) {
    list_ = list;
    node_ = nullptr;
}
template <typename Key, typename Comparator>
bool SkipList<Key, Comparator>::Iterator::Valid() const {
    return node_ != nullptr;
}
template <typename Key, typename Comparator>
Key const &SkipList<Key, Comparator>::Iterator::key() const {
    assert(Valid());
    return node_->key;
}
template <typename Key, typename Comparator>
void SkipList<Key, Comparator>::Iterator::Next() {
    assert(Valid());
    node_ = node_->Next(0);
}
template <typename Key, typename Comparator>
void SkipList<Key, Comparator>::Iterator::Prev() {
    assert(Valid());
    node_ = list_->FindLessThan(node_->key);
    if(node_ == list_->head_) {
        node_ = nullptr;
    }
}
template <typename Key, typename Comparator>
void SkipList<Key, Comparator>::Iterator::Seek(Key const &target) {
    node_ = list_->FindGreaterOrEqual(target, nullptr);
}
template <typename Key, typename Comparator>
void SkipList<Key, Comparator>::Iterator::SeekToFirst() {
    node_ = list_->head_->Next(0);
}
template <typename Key, typename Comparator>
void SkipList<Key, Comparator>::Iterator::SeekToLast() {
    node_ = list_->FindLast();
    if(node_ == list_->head_) {
        node_ = nullptr;
    }
}


} // ns_data_structure

#endif