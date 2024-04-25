# EncodeVarint32
每个字节的最高位（bit）有特殊含义
1. 如果最高位为 0，表示结束，当前字节的剩余 7 位就是该数据的表示。
    1. 表示整数1，需要一个字节：0 0000001
2. 如果最高位为 1，表示后续的字节也是该整型数据的一部分；
   1. 表示整数 300，需要两个字节：$1\ \  0101100\ \  0\ \  0000010$
      1. 组合起来就是 $(0000010\ \ 0101100)_2=300_{10}$

## 代码实现说明
```C++
uint8_t *EncodeVarint32(uint8_t *dst, uint32_t value) {
    static constexpr uint8_t B = 1 << 7; // 1 2 4 8 16 32 64 128, B = 128
    if (value < (1U << 7)) {
        *(dst++) = value;
    } else if (value < (1U << 14)) {
        // 将第8位置为1,表示后面还有字节,将uint32赋给uint8会截断前24位,留下最后8位
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
```