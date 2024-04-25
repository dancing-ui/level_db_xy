#include "no_destructor.h"
#include "log.h"
#include <gtest/gtest.h>

using namespace ns_util;

struct DoNotDestruct {
public:
    DoNotDestruct(uint32_t a, uint64_t b) :a(a), b(b) {}
    ~DoNotDestruct(){
        std::abort();
    }

    uint32_t a;
    uint64_t b;
};

constexpr uint32_t kA = 0xDEADBEEFU;
constexpr uint64_t kB = 0xAABBCCDDEEFFAABBU;

TEST(NoDestructorTest, StackInstance) {
    NoDestructor<DoNotDestruct> instance(kA, kB);
    ASSERT_EQ(kA, instance.get()->a);
    ASSERT_EQ(kB, instance.get()->b);
}

TEST(NoDestructorTest, StaticInstance) {
    static NoDestructor<DoNotDestruct> instance(kA, kB);
    ASSERT_EQ(kA, instance.get()->a);
    ASSERT_EQ(kB, instance.get()->b);
}

int main(int argc, char **argv) {
    PRINT_INFO("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}