#include "skip_list.h"
#include "random.h"
#include <set>
#include <gtest/gtest.h>
#include <cstdint>

using namespace ns_data_structure;
using namespace ns_memory;
using namespace ns_algorithm;
using namespace std;

using Key = uint64_t;
struct Comparator {
    int32_t operator()(Key const &a, Key const &b) const {
        if (a < b) {
            return -1;
        } else if (a > b) {
            return 1;
        }
        return 0;
    }
};

TEST(SkipTest, Empty) {
    Arena arena;
    Comparator cmp;
    SkipList<Key, Comparator> list(cmp, &arena);
    ASSERT_TRUE(!list.Contains(10));

    SkipList<Key, Comparator>::Iterator iter(&list);
    ASSERT_TRUE(!iter.Valid());
    iter.SeekToFirst();
    ASSERT_TRUE(!iter.Valid());
    iter.Seek(100);
    ASSERT_TRUE(!iter.Valid());
    iter.SeekToLast();
    ASSERT_TRUE(!iter.Valid());
}

TEST(SkipTest, InsertAndLookup) {
    const int N = 2000;
    const int R = 5000;
    Random rnd(1000);
    set<Key> keys;
    Arena arena;
    Comparator cmp;
    SkipList<Key, Comparator> list(cmp, &arena);
    for (int i = 0; i < N; i++) {
        Key key = rnd.Next() % R;
        /**
         * 本跳表仅支持唯一键码
         * 返回值second表示元素是否插入
         * 若为第一次插入，那么插入成功，返回true
         * 若为第二次插入，那么插入失败，返回false
         * */
        if (keys.insert(key).second) {
            list.Insert(key);
        }
    }

    for (int i = 0; i < R; i++) {
        if (list.Contains(i)) {
            ASSERT_EQ(keys.count(i), 1);
        } else {
            ASSERT_EQ(keys.count(i), 0);
        }
    }

    // Simple iterator tests
    {
        SkipList<Key, Comparator>::Iterator iter(&list);
        ASSERT_TRUE(!iter.Valid());

        iter.Seek(0);
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*(keys.begin()), iter.key());

        iter.SeekToFirst();
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*(keys.begin()), iter.key());

        iter.SeekToLast();
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*(keys.rbegin()), iter.key());
    }

    // forward iteration test
    {
        for (int i = 0; i < R; i++) {
            SkipList<Key, Comparator>::Iterator iter(&list);
            iter.Seek(i);

            set<Key>::iterator model_iter = keys.lower_bound(i);
            for (int j = 0; j < 3; j++) {
                if (model_iter == keys.end()) {
                    ASSERT_TRUE(!iter.Valid());
                    break;
                } else {
                    ASSERT_TRUE(iter.Valid());
                    ASSERT_EQ(*model_iter, iter.key());
                    ++model_iter;
                    iter.Next();
                }
            }
        }
    }

    // backward iteration test
    {
        SkipList<Key, Comparator>::Iterator iter(&list);
        iter.SeekToLast();
        for (std::set<Key>::reverse_iterator model_iter = keys.rbegin(); model_iter != keys.rend(); model_iter++) {
            ASSERT_TRUE(iter.Valid());
            ASSERT_EQ(*model_iter, iter.key());
            iter.Prev();
        }
        ASSERT_TRUE(!iter.Valid());
    }
}

int main(int argc, char **argv) {
    PRINT_INFO("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}