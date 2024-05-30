#include <gtest/gtest.h>
#include "log.h"
#include "table.h"

using namespace ns_util;

int main(int argc, char **argv) {
    PRINT_INFO("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}