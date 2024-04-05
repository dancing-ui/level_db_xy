#include "cache.h"
#include "thread_annotation.h"
#include "hash.h"

#include <mutex>
#include <iostream>

namespace ns_cache {

namespace {

struct LRUHandle {
    void *value;
    void (*deleter)(ns_data_structure::Slice const &key, void *value);
    LRUHandle *next_hash;
    LRUHandle *next;
    LRUHandle *prev;
    uint64_t charge;
    uint64_t key_length;
    bool in_cache;       // Whether entry is in the cache list.
    uint32_t refs;       // References, including cache reference, if present.
    uint32_t hash;       // Hash of key(); used for fast sharding and comparisons
    uint8_t key_data[1]; // Beginning of key

    ns_data_structure::Slice key() const {
        assert(next != this);
        return ns_data_structure::Slice(key_data, key_length);
    }
};
// We provide our own simple hash table since it removes a whole bunch
// of porting hacks and is also faster than some of the built-in hash
// table implementations in some of the compiler/runtime combinations
// we have tested.  E.g., readrandom speeds up by ~5% over the g++
// 4.4.3's builtin hashtable.
class HandleTable {
public:
    HandleTable() :
        length_(0), elems_(0), list_(nullptr) {
        Resize();
    }
    ~HandleTable() {
        delete[] list_;
    }

    LRUHandle *Lookup(ns_data_structure::Slice const &key, uint32_t hash) {
        return *FindPointer(key, hash);
    }

    LRUHandle *Insert(LRUHandle *h) {
        LRUHandle **ptr = FindPointer(h->key(), h->hash);
        LRUHandle *old = *ptr;
        h->next_hash = (old == nullptr ? nullptr : old->next_hash);
        *ptr = h;
        if (old == nullptr) {
            ++elems_;
            if (elems_ > length_) {
                Resize();
            }
        }
        return old;
    }

    LRUHandle *Remove(ns_data_structure::Slice const &key, uint32_t hash) {
        LRUHandle **ptr = FindPointer(key, hash);
        LRUHandle *result = *ptr;
        if (result != nullptr) {
            *ptr = result->next_hash;
            --elems_;
        }
        return result;
    }

private:
    // The table consists of an array of buckets where each bucket is
    // a linked list of cache entries that hash into the bucket.
    uint32_t length_;
    uint32_t elems_;
    LRUHandle **list_;

    LRUHandle **FindPointer(ns_data_structure::Slice const &key, uint32_t hash) {
        LRUHandle **ptr = &list_[hash & (length_ - 1)];
        while (*ptr != nullptr && ((*ptr)->hash != hash || key != (*ptr)->key())) {
            ptr = &(*ptr)->next_hash;
        }
        return ptr;
    }

    void Resize() {
        uint32_t new_length = 4;
        while (new_length < elems_) {
            new_length *= 2;
        }
        LRUHandle **new_list = new LRUHandle *[new_length];
        memset(new_list, 0, sizeof(new_list[0]) * new_length);
        uint32_t count = 0;
        for (uint32_t i = 0; i < length_; i++) {
            LRUHandle *h = list_[i];
            while (h != nullptr) {
                LRUHandle *next = h->next_hash;
                uint32_t hash = h->hash;
                LRUHandle **ptr = &new_list[hash & (new_length - 1)];
                h->next_hash = *ptr;
                *ptr = h;
                h = next;
                count++;
            }
        }
        assert(elems_ == count);
        delete[] list_;
        list_ = new_list;
        length_ = new_length;
    }
};

class LRUCache {
public:
    LRUCache();
    ~LRUCache();

    void SetCapacity(uint64_t capacity) {
        capacity_ = capacity;
    }

    Cache::Handle *Insert(ns_data_structure::Slice const &key, uint32_t hash, void *value, uint64_t charge, void (*deleter)(ns_data_structure::Slice const &key, void *value));
    Cache::Handle *Lookup(ns_data_structure::Slice const &key, uint32_t hash);
    void Release(Cache::Handle *handle);
    void Erase(ns_data_structure::Slice const &key, uint32_t hash);
    void Prune();
    uint64_t TotalCharge() const {
        std::unique_lock<std::mutex> lck(mutex_);
        return usage_;
    }

private:
    // Initialized before use.
    uint64_t capacity_;
    mutable std::mutex mutex_;
    uint64_t usage_ GUARDED_BY(mutex_);
    LRUHandle lru_ GUARDED_BY(mutex_);
    LRUHandle in_use_ GUARDED_BY(mutex_);
    HandleTable table_ GUARDED_BY(mutex_);

    void LRU_Remove(LRUHandle *e);
    void LRU_Append(LRUHandle *list, LRUHandle *e);
    void Ref(LRUHandle *e);
    void Unref(LRUHandle *e);
    void FinishErase(LRUHandle *e) EXCLUSIVE_LOCKS_REQUIRED(mutex_);
};

LRUCache::LRUCache() :
    capacity_(0), usage_(0) {
    // Make empty circular linked lists.
    lru_.next = &lru_;
    lru_.prev = &lru_;
    in_use_.next = &in_use_;
    in_use_.prev = &in_use_;
}

LRUCache::~LRUCache() {
    assert(in_use_.next == &in_use_); // Error if caller has an unreleased handle
    for (LRUHandle *e = lru_.next; e != &lru_;) {
        LRUHandle *next = e->next;
        assert(e->in_cache);
        e->in_cache = false;
        assert(e->refs == 1); // Invariant of lru_ list.
        Unref(e);
        e = next;
    }
}

Cache::Handle *LRUCache::Insert(ns_data_structure::Slice const &key, uint32_t hash, void *value, uint64_t charge, void (*deleter)(ns_data_structure::Slice const &key, void *value)) {
    std::unique_lock<std::mutex> lck(mutex_);
    LRUHandle *e = reinterpret_cast<LRUHandle *>(malloc(sizeof(LRUHandle) - 1 + key.size()));
    e->value = value;
    e->deleter = deleter;
    e->charge = charge;
    e->key_length = key.size();
    e->hash = hash;
    e->in_cache = false;
    e->refs = 1; // for the returned handle.
    std::memcpy(e->key_data, key.data(), key.size());
    if (capacity_ > 0) {
        e->refs++; // for the cache's reference.
        e->in_cache = true;
        LRU_Append(&in_use_, e);
        usage_ += charge;
        FinishErase(table_.Insert(e));
    } else { // don't cache. (capacity_==0 is supported and turns off caching.)
             // next is read by key() in an assert, so it must be initialized
        e->next = nullptr;
    }
    while (usage_ > capacity_ && lru_.next != &lru_) {
        LRUHandle *old = lru_.next;
        assert(old->refs == 1);
        FinishErase(table_.Remove(old->key(), old->hash));
    }
    return reinterpret_cast<Cache::Handle *>(e);
}

Cache::Handle *LRUCache::Lookup(ns_data_structure::Slice const &key, uint32_t hash) {
    std::unique_lock<std::mutex> lck(mutex_);
    LRUHandle *e = table_.Lookup(key, hash);
    if (e != nullptr) {
        Ref(e);
    }
    return reinterpret_cast<Cache::Handle *>(e);
}

void LRUCache::Release(Cache::Handle *handle) {
    std::unique_lock<std::mutex> lck(mutex_);
    Unref(reinterpret_cast<LRUHandle *>(handle));
}

void LRUCache::Erase(ns_data_structure::Slice const &key, uint32_t hash) {
    std::unique_lock<std::mutex> lck(mutex_);
    FinishErase(table_.Remove(key, hash));
}

void LRUCache::Prune() {
    std::unique_lock<std::mutex> lck(mutex_);
    while (lru_.next != &lru_) {
        LRUHandle *e = lru_.next;
        assert(e->refs == 1);
        FinishErase(table_.Remove(e->key(), e->hash));
    }
}

void LRUCache::LRU_Remove(LRUHandle *e) {
    e->next->prev = e->prev;
    e->prev->next = e->next;
}

void LRUCache::LRU_Append(LRUHandle *list, LRUHandle *e) {
    // Make "e" newest entry by inserting just before *list
    e->next = list;
    e->prev = list->prev;
    e->prev->next = e;
    e->next->prev = e;
}

void LRUCache::Ref(LRUHandle *e) {
    if (e->refs == 1 && e->in_cache) { // If on lru_ list, move to in_use_ list.
        LRU_Remove(e);
        LRU_Append(&in_use_, e);
    }
    e->refs++;
}

void LRUCache::Unref(LRUHandle *e) {
    assert(e->refs > 0);
    e->refs--;
    if (e->refs == 0) { // Deallocate.
        assert(!e->in_cache);
        e->deleter(e->key(), e->value);
        free(e);
    } else if (e->in_cache && e->refs == 1) {
        // No longer in use; move to lru_ list.
        LRU_Remove(e);
        LRU_Append(&lru_, e);
    }
}

void LRUCache::FinishErase(LRUHandle *e) {
    if (e != nullptr) {
        assert(e->in_cache);
        LRU_Remove(e);
        e->in_cache = false;
        usage_ -= e->charge;
        Unref(e);
    }
}

static constexpr int32_t kNumShardBits = 4;
static constexpr int32_t kNumShards = 1 << kNumShardBits;

class ShardedLRUCache : public Cache {
public:
    explicit ShardedLRUCache(uint64_t capacaity) :
        last_id_(0) {
        uint64_t const per_shard = (capacaity + (kNumShards - 1)) / kNumShards;
        for (int32_t s = 0; s < kNumShards; s++) {
            shard_[s].SetCapacity(per_shard);
        }
    }
    ~ShardedLRUCache() override {
    }
    Handle *Insert(ns_data_structure::Slice const &key, void *value, uint64_t charge,
                   void (*deleter)(ns_data_structure::Slice const &key, void *value)) override {
        uint32_t const hash = HashSlice(key);
        return shard_[Shard(hash)].Insert(key, hash, value, charge, deleter);
    }

    Handle *Lookup(ns_data_structure::Slice const &key) override {
        uint32_t const hash = HashSlice(key);
        return shard_[Shard(hash)].Lookup(key, hash);
    }

    void Release(Handle *handle) override {
        LRUHandle *h = reinterpret_cast<LRUHandle *>(handle);
        shard_[Shard(h->hash)].Release(handle);
    }

    void *Value(Handle *handle) override {
        return reinterpret_cast<LRUHandle *>(handle)->value;
    }

    void Erase(ns_data_structure::Slice const &key) override {
        uint32_t const hash = HashSlice(key);
        shard_[Shard(hash)].Erase(key, hash);
    }

    uint64_t NewId() override {
        std::unique_lock<std::mutex> lck(id_mutex_);
        return ++(last_id_);
    }

    void Prune() override {
        for (int32_t s = 0; s < kNumShards; s++) {
            shard_[s].Prune();
        }
    }

    uint64_t TotalCharge() const override {
        uint64_t total = 0;
        for (int32_t s = 0; s < kNumShards; s++) {
            total += shard_[s].TotalCharge();
        }
        return total;
    }

private:
    LRUCache shard_[kNumShards];
    std::mutex id_mutex_;
    uint64_t last_id_;

    static inline uint32_t HashSlice(ns_data_structure::Slice const &s) {
        return ns_util::Hash(s.data(), s.size(), 0);
    }

    static uint32_t Shard(uint32_t hash) {
        return hash >> (32 - kNumShardBits);
    }
};

} // anonymous namespace
Cache *NewLRUCache(uint64_t capacity) {
    return new ShardedLRUCache(capacity);
}
} // ns_cache