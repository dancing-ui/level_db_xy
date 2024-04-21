本次使用google的crc32c，[链接](https://github.com/google/crc32c/tree/1.1.2)

使用check_library_exists检查crc32c库中crc32c_value函数是否存在
```CMake
include(CheckLibraryExists)
check_library_exists(crc32c crc32c_value "" HAVE_CRC32C)
```
安装crc32c的过程如下：
```sh
git clone git@github.com:google/crc32c.git
cd crc32c
git submodule update --init --recursive
git checkout 1.1.2
git pull origin 1.1.2
mkdir build
cd build
cmake -DCRC32C_BUILD_TESTS=0 -DCRC32C_BUILD_BENCHMARKS=0 .. 
make -j20
sudo make install
```

# RoundUp函数

- template \<int N>
  - 这是一个模板参数，表示我们想要向上舍入到的倍数。N一般是2的倍数
- constexpr inline uint8_t const *RoundUp(uint8_t const *pointer)
  - 这是函数的声明，它接受一个const uint8_t*类型的指针，并返回一个同类型的指针。
  - constexpr表示该函数可以在编译时计算
  - inline建议编译器内联该函数以减少函数调用的开销。
- reinterpret_cast<uintptr_t>(pointer)
  - 这将pointer转换为uintptr_t类型，uintptr_t是一个无符号整数类型，其大小足以存储指针值。
  - 这样做是为了能够对指针地址进行算术运算。
- ~static_cast<uintptr_t>(N - 1)
  - ~static_cast<uintptr_t>(N - 1)计算出一个掩码，该掩码的所有位都是1，除了最低的几位（取决于N的值）是0。然后，使用按位与操作&，将上一步的结果与这个掩码相与，从而将不是N倍数的部分清零。
- (reinterpret_cast<uintptr_t>(pointer) + (N - 1)) & ~static_cast<uintptr_t>(N - 1)
  - 将指针地址加上N-1，是为了确保如果原始指针不是N的倍数，那么加上 N - 1 后的值至少会超过下一个N的倍数，然后位掩码会将其向下对齐。如果原始指针已经是N的倍数，加上 N - 1 后的值会在同一个N的倍数范围内，位掩码操作仍然会得到原始的N的倍数
  - & ~static_cast<uintptr_t>(N - 1)
    - 这是一个位掩码操作，用于将上一步得到的数值向下舍入到N的最近倍数。
    - 例如，如果N是16，那么N - 1是15，其二进制表示是00001111。取反后得到的掩码是11110000。这样，任何地址都会被舍入到16的倍数，因为最后四位将被清零。
  - 举个例子
    - 若reinterpret_cast<uintptr_t>(pointer) = 3， N = 4，则(reinterpret_cast<uintptr_t>(pointer) + (N - 1)) & ~static_cast<uintptr_t>(N - 1) = $6_{10}$ & $(111100)_2$ = $(100)_2$=$4_{10}$
- reinterpret_cast<uint8_t *>(...)
  - 最后，将结果转换回uint8_t*类型的指针。

该函数可以用于确保数据结构或缓冲区在内存中的对齐，这对于提高某些处理器的性能非常重要。

## 为什么内存对齐，需要把地址的最后几位清零？
- 将地址的最后几位清零是一种确保地址对齐的技术。这是因为对齐的地址意味着它是某个特定值（对齐值）的倍数。在二进制中，如果一个数是2的幂的倍数，那么它的二进制表示中从最低位开始的几位将是0。例如，如果一个地址是4的倍数（2的2次幂），那么它的二进制表示的最后两位将是00；如果是8的倍数（2的3次幂），最后三位将是000，以此类推。
- 通过清零地址的最后几位，我们可以确保地址是对齐值的倍数。例如，如果对齐值是16（即2的4次幂），我们需要确保地址的最后四位是0。这样，无论原始地址是什么，经过清零操作后的地址都将是16的倍数，从而满足对齐要求。