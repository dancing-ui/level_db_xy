#include "log.h"
#include "write_batch.h"
#include "db_format.h"
#include "mem_table.h"
#include "write_batch_internal.h"
#include "iterator.h"
#include "logging.h"

#include <gtest/gtest.h>

using namespace ns_db_format;
using namespace ns_data_structure;
using namespace ns_write_batch;
using namespace ns_comparator;
using namespace ns_util;
using namespace ns_iterator;
using namespace ns_log;

static std::string PrintContents(WriteBatch *b) {
    InternalKeyComparator cmp(BytewiseComparator());
    MemTable *mem = new MemTable(cmp);
    mem->Ref();
    std::string state;
    Status s = WriteBatchInternal::InsertInto(b, mem);
    int32_t count = 0;
    Iterator *iter = mem->NewIterator();
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        ParsedInternalKey ikey;
        EXPECT_TRUE(ParseInternalKey(iter->key(), &ikey));
        switch (ikey.type) {
        case kTypeValue:
            state.append("Put(");
            state.append(ikey.user_key.ToString());
            state.append(", ");
            state.append(iter->value().ToString());
            state.append(")");
            count++;
            break;
        case kTypeDeletion:
            state.append("Delete(");
            state.append(ikey.user_key.ToString());
            state.append(")");
            count++;
            break;
        }
        state.append("@");
        state.append(NumberToString(ikey.sequence));
    }
    delete iter;
    if (!s.ok()) {
        state.append("ParseError()");
    } else if (count != WriteBatchInternal::Count(b)) {
        state.append("CountMismatch()");
    }
    mem->Unref();
    return state;
}

TEST(WriteBatchTest, Empty) {
    WriteBatch b;
    ASSERT_EQ("", PrintContents(&b));
    ASSERT_EQ(0, WriteBatchInternal::Count(&b));
}

TEST(WriteBatchTest, Multiple) {
    WriteBatch b;
    b.Put(Slice("foo"), Slice("bar"));
    b.Delete(Slice("box"));
    b.Put(Slice("baz"), Slice("boo"));
    WriteBatchInternal::SetSequence(&b, 100);
    ASSERT_EQ(100, WriteBatchInternal::Sequence(&b));
    ASSERT_EQ(3, WriteBatchInternal::Count(&b));
    ASSERT_EQ(
        "Put(baz, boo)@102"
        "Delete(box)@101"
        "Put(foo, bar)@100",
        PrintContents(&b)
    );
}

TEST(WriteBatchTest, Corruption) {
    WriteBatch b;
    b.Put(Slice("foo"), Slice("bar"));
    b.Delete(Slice("box"));
    WriteBatchInternal::SetSequence(&b, 200);
    Slice contents = WriteBatchInternal::Contents(&b);
    WriteBatchInternal::SetContents(&b, Slice(contents.data(), contents.size() - 1));
    ASSERT_EQ(
        "Put(foo, bar)@200"
        "ParseError()",
        PrintContents(&b)
    );
}

TEST(WriteBatchTest, Append) {
    WriteBatch b1, b2;
    WriteBatchInternal::SetSequence(&b1, 200);
    WriteBatchInternal::SetSequence(&b2, 300);
    b1.Append(b2);
    ASSERT_EQ("", PrintContents(&b1));
    b2.Put("a", "va");
    b1.Append(b2);
    ASSERT_EQ("Put(a, va)@200", PrintContents(&b1));
    b2.Clear();
    b2.Put("b", "vb");
    b1.Append(b2);
    ASSERT_EQ(
        "Put(a, va)@200"
        "Put(b, vb)@201",
        PrintContents(&b1)  
    );
    b2.Delete("foo");
    b1.Append(b2);
    ASSERT_EQ(
        "Put(a, va)@200"
        "Put(b, vb)@202"
        "Put(b, vb)@201"
        "Delete(foo)@203",
        PrintContents(&b1)
    );
}

TEST(WriteBatchTest, ApproximateSize) {
    WriteBatch b;
    uint64_t empty_size = b.ApproximateSize();
    b.Put(Slice("foo"), Slice("bar"));
    uint64_t one_key_size = b.ApproximateSize();
    ASSERT_LT(empty_size, one_key_size);
    b.Put(Slice("baz"), Slice("boo"));
    uint64_t two_key_size = b.ApproximateSize();
    ASSERT_LT(one_key_size, two_key_size);
    b.Delete(Slice("box"));
    uint64_t post_delete_size = b.ApproximateSize();
    ASSERT_LT(two_key_size, post_delete_size);
}

int main(int argc, char **argv) {
    PRINT_INFO("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}