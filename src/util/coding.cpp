#include "coding.h"

namespace ns_util {

void PutFixed64(std::string *dst, uint64_t value) {
    uint8_t buf[sizeof(value)];
    EncodeFixed64(buf, value);
    dst->append(reinterpret_cast<char *>(buf), sizeof(buf));
}

uint8_t *EncodeVarint32(uint8_t *dst, uint32_t value) {
    /**
     * 每个字节的最高位（bit）有特殊含义
     * 如果该位为 0，表示结束，当前字节的剩余 7 位就是该数据的表示。
     *      表示整数 1，需要一个字节：0000 0001
     * 如果该位为 1，表示后续的字节也是该整型数据的一部分；
     *      表示整数 300，需要两个字节：1010 1100 0000 0010
    */
    static constexpr uint8_t B = 1 << 7;
    if (value < (1U << 7)) {
        *(dst++) = value;
    } else if (value < (1U << 14)) {
        *(dst++) = value | B;
        *(dst++) = value >> 7;
    } else if (value < (1U << 21)) {
        *(dst++) = value | B;
        *(dst++) = (value >> 7) | B;
        *(dst++) = value >> 14;
    } else if (value < (1ULL << 28)) {
        *(dst++) = value | B;
        *(dst++) = (value >> 7) | B;
        *(dst++) = (value >> 14) | B;
        *(dst++) = value >> 21;
    } else {
        *(dst++) = value | B;
        *(dst++) = (value >> 7) | B;
        *(dst++) = (value >> 14) | B;
        *(dst++) = (value >> 21) | B;
        *(dst++) = value >> 28;
    }
    return dst;
}

uint8_t const *GetVarint32Ptr(uint8_t const *p, uint8_t const *limit, uint32_t *value) {
    uint32_t result{0U};
    static constexpr uint8_t B = 1 << 7;
    for (uint32_t shift = 0U; shift <= 28 && p < limit; shift += 7) {
        uint32_t byte = *p;
        p++;
        if (byte & B) {
            result |= ((byte & 127) << shift);
        } else {
            result |= (byte << shift);
            *value = result;
            return p;
        }
    }
    return nullptr;
}

} // ns_util