#include <gtest/gtest.h>
#include "log.h"
#include "hash.h"

using namespace ns_util;

TEST(HASH, SignedUnsignedIssue) {
    const uint8_t data1[1] = {0x62};
    const uint8_t data2[2] = {0xC3, 0x97};
    const uint8_t data3[3] = {0xE2, 0x99, 0xA5};
    const uint8_t data4[4] = {0xE1, 0x80, 0xB9, 0x32};
    const uint8_t data5[48] = {
        0x01, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
        0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x18, 0x28, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    ASSERT_EQ(Hash(0, 0, 0xBC9F1D34), 0xBC9F1D34);
    ASSERT_EQ(Hash(data1, sizeof(data1), 0xBC9F1D34), 0xEF1345C4);
    ASSERT_EQ(Hash(data2, sizeof(data2), 0xBC9F1D34), 0x5B663814);
    ASSERT_EQ(Hash(data3, sizeof(data3), 0xBC9F1D34), 0x323C078F);
    ASSERT_EQ(Hash(data4, sizeof(data4), 0xBC9F1D34), 0xED21633A);
    ASSERT_EQ(Hash(data5, sizeof(data5), 0x12345678), 0xF333DABB);
    
}

int main(int argc, char **argv) {
    PRINT_INFO("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}