#include "compression.h"
#include "log.h"
#include <gtest/gtest.h>

using namespace ns_compression;

TEST(Zstd, Zstd_Compress) {
    
}

int main(int argc, char **argv) {
    PRINT_INFO("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}