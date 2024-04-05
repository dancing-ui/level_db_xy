#ifndef _LEVEL_DB_XY_CACHE_H_
#define _LEVEL_DB_XY_CACHE_H_

#include "slice.h"
#include <functional>

namespace ns_cache {

class Cache {
public:
    Cache() = default;
    Cache(Cache const &) = delete;
    Cache &operator=(Cache const &) = delete;
    // Destroys all existing entries by calling the "deleter"
    // function that was passed to the constructor.
    virtual ~Cache() = default;
    // Opaque handle to an entry stored in the cache.
    struct Handle {};
    // Insert a mapping from key->value into the cache and assign it
    // the specified charge against the total cache capacity.
    //
    // Returns a handle that corresponds to the mapping.  The caller
    // must call this->Release(handle) when the returned mapping is no
    // longer needed.
    //
    // When the inserted entry is no longer needed, the key and
    // value will be passed to "deleter".
    virtual Handle *Insert(ns_data_structure::Slice const &key, void *value, uint64_t charge,
                           void (*deleter)(ns_data_structure::Slice const &key, void *value)) = 0;
    // If the cache has no mapping for "key", returns nullptr.
    //
    // Else return a handle that corresponds to the mapping.  The caller
    // must call this->Release(handle) when the returned mapping is no
    // longer needed.
    virtual Handle *Lookup(ns_data_structure::Slice const &key) = 0;
    // Release a mapping returned by a previous Lookup().
    // REQUIRES: handle must not have been released yet.
    // REQUIRES: handle must have been returned by a method on *this.
    virtual void Release(Handle *handle) = 0;
    // Return the value encapsulated in a handle returned by a
    // successful Lookup().
    // REQUIRES: handle must not have been released yet.
    // REQUIRES: handle must have been returned by a method on *this.
    virtual void *Value(Handle *handle) = 0;
    // If the cache contains entry for key, erase it.  Note that the
    // underlying entry will be kept around until all existing handles
    // to it have been released.
    virtual void Erase(ns_data_structure::Slice const &key) = 0;
    // Return a new numeric id.  May be used by multiple clients who are
    // sharing the same cache to partition the key space.  Typically the
    // client will allocate a new id at startup and prepend the id to
    // its cache keys.
    virtual uint64_t NewId() = 0;
    // Remove all cache entries that are not actively in use.  Memory-constrained
    // applications may wish to call this method to reduce memory usage.
    // Default implementation of Prune() does nothing.  Subclasses are strongly
    // encouraged to override the default implementation.  A future release of
    // leveldb may change Prune() to a pure abstract method.
    virtual void Prune() {
    }
    // Return an estimate of the combined charges of all elements stored in the
    // cache.
    virtual uint64_t TotalCharge() const = 0;
};

Cache *NewLRUCache(uint64_t capacity);

} // ns_cache

#endif