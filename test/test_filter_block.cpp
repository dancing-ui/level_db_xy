#include "crc32c.h"
#include "log.h"
#include "filter_block.h"
#include "filter_policy.h"
#include "hash.h"
#include "coding.h"

#include <gtest/gtest.h>

using namespace ns_util;
using namespace ns_table;
using namespace ns_filter_policy;
using namespace ns_data_structure;

// For testing: emit an array with one hash value per key
class TestHashFilter : public FilterPolicy {
public:
    const char *Name() const override {
        return "TestHashFilter";
    }

    void CreateFilter(const Slice *keys, int n, std::string *dst) const override {
        for (int i = 0; i < n; i++) {
            uint32_t h = Hash(keys[i].data(), keys[i].size(), 1);
            PutFixed32(dst, h);
        }
    }

    bool KeyMayMatch(const Slice &key, const Slice &filter) const override {
        uint32_t h = Hash(key.data(), key.size(), 1);
        for (size_t i = 0; i + 4 <= filter.size(); i += 4) {
            if (h == DecodeFixed32(filter.data() + i)) {
                return true;
            }
        }
        return false;
    }
};

class FilterBlockTest : public testing::Test {
public:
    TestHashFilter policy_;
};

TEST_F(FilterBlockTest, SingleChunk) {
    FilterBlockBuilder builder(&policy_);
    builder.StartBlock(100);
    builder.AddKey("foo");
    builder.AddKey("bar");
    builder.AddKey("box");
    builder.StartBlock(200);
    builder.AddKey("box");
    builder.StartBlock(300);
    builder.AddKey("hello");
    Slice block = builder.Finish();
    FilterBlockReader reader(&policy_, block);
    ASSERT_TRUE(reader.KeyMayMatch(100, "foo"));
    ASSERT_TRUE(reader.KeyMayMatch(100, "bar"));
    ASSERT_TRUE(reader.KeyMayMatch(100, "box"));
    ASSERT_TRUE(reader.KeyMayMatch(100, "hello"));
    ASSERT_TRUE(reader.KeyMayMatch(100, "foo"));
    ASSERT_TRUE(!reader.KeyMayMatch(100, "missing"));
    ASSERT_TRUE(!reader.KeyMayMatch(100, "other"));
}

int main(int argc, char **argv) {
    PRINT_INFO("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}