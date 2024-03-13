#include "db_format.h"
#include "log.h"
#include "comparator.h"

#include <gtest/gtest.h>

using namespace ns_db_format;
using namespace ns_comparator;
using namespace ns_data_structure;

std::string IKey(std::string const &user_key, uint64_t seq, ValueType vt) {
    std::string encoded;
    AppendInternalKey(&encoded, ParsedInternalKey(user_key, seq, vt));
    return encoded;
}

std::string Shorten(std::string const &s, std::string const &l) {
    std::string result = s;
    InternalKeyComparator(BytewiseComparator()).FindShortestSeparator(&result, l);
    return result;
}

std::string ShortSuccessor(std::string const &s) {
    std::string result = s;
    InternalKeyComparator(BytewiseComparator()).FindShortSuccessor(&result);
    return result;
}

void TestKey(std::string const &key, uint64_t seq, ValueType vt) {
    std::string encoded = IKey(key, seq, vt);

    Slice in(encoded);
    ParsedInternalKey decoded("", 0, ValueType::kTypeValue);

    ASSERT_TRUE(ParseInternalKey(in, &decoded));
    ASSERT_EQ(key, decoded.user_key.ToString());
    ASSERT_EQ(seq, decoded.sequence);
    ASSERT_EQ(vt, decoded.type);

    ASSERT_TRUE(!ParseInternalKey(Slice("bar"), &decoded));
}

TEST(FormatTest, InternalKey_EncodeDecode) {
    char const *keys[] = {"", "k", "hello", "longggggggggggggggggggg"};
    uint64_t const seq[] = {1,
                            2,
                            3,
                            (1ULL << 8) - 1,
                            1ULL << 8,
                            (1ULL << 8) + 1,
                            (1ULL << 16) - 1,
                            1ULL << 16,
                            (1ULL << 16) + 1,
                            (1ULL << 32) - 1,
                            1ULL << 32,
                            (1ULL << 32) + 1};
    for(int k=0;k<sizeof(keys) / sizeof(keys[0]);k++) {
        for(int s=0;s<sizeof(seq)/sizeof(seq[0]);s++) {
            TestKey(keys[k], seq[s], ValueType::kTypeValue);
            TestKey("hello", 1, ValueType::kTypeDeletion);
        }
    }
}

int main(int argc, char **argv) {
    PRINT_INFO("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}