# EncodeVarint32
每个字节的最高位（bit）有特殊含义
1. 如果最高位为 0，表示结束，当前字节的剩余 7 位就是该数据的表示。
    1. 表示整数1，需要一个字节：0 0000001
2. 如果最高位为 1，表示后续的字节也是该整型数据的一部分；
   1. 表示整数 300，需要两个字节：$1\ \  0101100\ \  0\ \  0000010$
      1. 组合起来就是 $(0000010\ \ 0101100)_2=300_{10}$