#include "db_format.h"
#include "log.h"
#include "comparator.h"
#include "cache.h"
#include <gtest/gtest.h>
#include <vector>

using namespace ns_db_format;
using namespace ns_comparator;
using namespace ns_data_structure;
using namespace ns_util;
using namespace ns_cache;

static std::string EncodeKey(int32_t k) {
    std::string result;
    PutFixed32(&result, k);
    return result;
}

static int32_t DecodeKey(Slice const &k) {
    assert(k.size() == 4);
    return DecodeFixed32(k.data());
}

static void *EncodeValue(uintptr_t v) {
    return reinterpret_cast<void *>(v);
}

static int32_t DecodeValue(void *v) {
    return reinterpret_cast<uintptr_t>(v);
}

class CacheTest : public testing::Test {
public:
    static void Deleter(Slice const &key, void *v) {
        currrent_->deleted_keys_.push_back(DecodeKey(key));
        currrent_->deleted_values_.push_back(DecodeValue(v));
    };

    static constexpr int32_t kCacheSize = 1000;
    std::vector<int32_t> deleted_keys_;
    std::vector<int32_t> deleted_values_;
    Cache *cache_;

    CacheTest() :
        cache_(NewLRUCache(kCacheSize)) {
        currrent_ = this;
    }

    ~CacheTest() {
        delete cache_;
    }

    int32_t Lookup(int32_t key) {
        Cache::Handle *handle = cache_->Lookup(EncodeKey(key));
        int32_t const r = (handle == nullptr) ? -1 : DecodeValue(cache_->Value(handle));
        if (handle != nullptr) {
            cache_->Release(handle);
        }
        return r;
    }

    void Insert(int32_t key, int32_t value, int32_t charge = 1) {
        cache_->Release(cache_->Insert(EncodeKey(key), EncodeValue(value), charge, CacheTest::Deleter));
    }

    Cache::Handle *InsertAndReturnHandle(int32_t key, int32_t value, int32_t charge = 1) {
        return cache_->Insert(EncodeKey(key), EncodeValue(value), charge, CacheTest::Deleter);
    }

    void Erase(int32_t key) {
        cache_->Erase(EncodeKey(key));
    }

    static CacheTest *currrent_;
};

CacheTest *CacheTest::currrent_;

TEST_F(CacheTest, HitAndMiss) {
    ASSERT_EQ(-1, Lookup(100));

    Insert(100, 101);
    ASSERT_EQ(101, Lookup(100));
    ASSERT_EQ(-1, Lookup(200));
    ASSERT_EQ(-1, Lookup(300));

    Insert(200, 201);
    ASSERT_EQ(101, Lookup(100));
    ASSERT_EQ(201, Lookup(200));
    ASSERT_EQ(-1, Lookup(300));

    Insert(100, 102);
    ASSERT_EQ(102, Lookup(100));
    ASSERT_EQ(201, Lookup(200));
    ASSERT_EQ(-1, Lookup(300));

    ASSERT_EQ(1, deleted_keys_.size());
    ASSERT_EQ(100, deleted_keys_[0]);
    ASSERT_EQ(101, deleted_values_[0]);
}

TEST_F(CacheTest, Erase) {
    Erase(200);
    ASSERT_EQ(0, deleted_keys_.size());

    Insert(100, 101);
    Insert(200, 201);
    Erase(100);
    ASSERT_EQ(-1, Lookup(100));
    ASSERT_EQ(201, Lookup(200));
    ASSERT_EQ(1, deleted_keys_.size());
    ASSERT_EQ(100, deleted_keys_[0]);
    ASSERT_EQ(101, deleted_values_[0]);

    Erase(100);
    ASSERT_EQ(-1, Lookup(100));
    ASSERT_EQ(201, Lookup(200));
    ASSERT_EQ(1, deleted_keys_.size());
}

TEST_F(CacheTest, EntireArePinned) {
    Insert(100, 101);
    Cache::Handle *h1 = cache_->Lookup(EncodeKey(100));
    ASSERT_EQ(101, DecodeValue(cache_->Value(h1)));

    Insert(100, 102);
    Cache::Handle *h2 = cache_->Lookup(EncodeKey(100));
    ASSERT_EQ(102, DecodeValue(cache_->Value(h2)));
    ASSERT_EQ(0, deleted_keys_.size());

    cache_->Release(h1);
    ASSERT_EQ(1, deleted_keys_.size());
    ASSERT_EQ(100, deleted_keys_[0]);
    ASSERT_EQ(101, deleted_values_[0]);

    Erase(100);
    ASSERT_EQ(-1, Lookup(100));
    ASSERT_EQ(1, deleted_keys_.size());

    cache_->Release(h2);
    ASSERT_EQ(2, deleted_keys_.size());
    ASSERT_EQ(100, deleted_keys_[1]);
    ASSERT_EQ(102, deleted_values_[1]);
}

TEST_F(CacheTest, EvictionPolicy) {
    Insert(100, 101);
    Insert(200, 201);
    Insert(300, 301);
    Cache::Handle *h = cache_->Lookup(EncodeKey(300));
    for (int32_t i = 0; i < kCacheSize + 100; i++) {
        Insert(1000 + i, 2000 + i);
        ASSERT_EQ(2000 + i, Lookup(1000 + i));
        ASSERT_EQ(101, Lookup(100));
    }
    ASSERT_EQ(101, Lookup(100));
    ASSERT_EQ(-1, Lookup(200));
    ASSERT_EQ(301, Lookup(300));
    cache_->Release(h);
}

int main(int argc, char **argv) {
    PRINT_INFO("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}