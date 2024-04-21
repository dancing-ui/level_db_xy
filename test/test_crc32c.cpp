#include "crc32c.h"
#include "log.h"

#include <gtest/gtest.h>

using namespace ns_util;

TEST(Crc32C, StandardResults) {
    uint8_t buf[32];
    memset(buf, 0, sizeof(buf));
    ASSERT_EQ(0x8A9136AA, Value(buf, sizeof(buf))); 

    memset(buf, 0xFF, sizeof(buf));
    ASSERT_EQ(0x62A8AB43, Value(buf, sizeof(buf)));

    for(int32_t i = 0;i<32;i++) {
        buf[i] = i;
    }
    ASSERT_EQ(0x46DD794E, Value(buf, sizeof(buf)));

    for(int32_t i =0;i<32;i++) {
        buf[i] = 31 - i;
    }
    ASSERT_EQ(0x113FDB5C, Value(buf, sizeof(buf)));

    uint8_t data[48] = {
      0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x18, 0x28, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  ASSERT_EQ(0xD9963A56, Value(data, sizeof(data)));
}

TEST(Crc32C, Values) {
    ASSERT_NE(Value(reinterpret_cast<uint8_t const*>("a"), 1), Value(reinterpret_cast<uint8_t const*>("foo"), 3));
}

TEST(Crc32C, Extend) {
    ASSERT_EQ(Value(reinterpret_cast<uint8_t const*>("hello world"), 11), Extend(Value(reinterpret_cast<uint8_t const*>("hello "), 6), reinterpret_cast<uint8_t const*>("world"), 5));
}

TEST(Crc32C, Mask) {
    uint32_t crc = Value(reinterpret_cast<uint8_t const*>("foo"), 3);
    ASSERT_NE(crc, Mask(crc));
    ASSERT_NE(crc, Mask(Mask(crc)));
    ASSERT_EQ(crc, Unmask(Mask(crc)));
    ASSERT_EQ(crc, Unmask(Unmask(Mask(Mask(crc)))));
}

int main(int argc, char **argv) {
    PRINT_INFO("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}