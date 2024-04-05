[布隆过滤器](https://cloud.tencent.com/developer/article/1882907)

在介绍布隆过滤器之前，先介绍下 LevelDB 中的过滤器策略 FilterPolicy。考虑一个场景：在 LevelDB 中查询某个指定 key = query 对应的 value，如果我们事先知道了所有的 key 里都找不到这个 query，那也就不需要进一步的读取磁盘、精确查找了，可以有效地减少磁盘访问数量。

FilterPolicy 就负责这件事情：它可以根据一组 key 创建一个小的过滤器 filter，并且可以将该过滤器和键值对存储在磁盘中，在查询时快速判断 query 是否在 filter 中*【个人理解就是C++里面的std::set】*。默认使用的 FilterPolicy 即为布隆过滤器。

FilterPolicy 中，CreateFilter 负责创建 filter，KeyMayMatch 负责判断 key 是否在 filter 中。注意这个 May，即这里 Match 判断可能会出错，也允许会出错。对于布隆过滤器，如果 Key 在 filter 里，那么一定会 Match 上；反之如果不在，那么有小概率也会 Match 上，进而会多做一些磁盘访问，只要这个概率足够小也无伤大雅。这也刚好符合 KeyMayMatch 函数的需求，有兴趣可以看原代码中的英文注释。

暴露的接口除了 FilterPolicy 接口类，还有 NewBloomFilterPolicy 函数，其他代码中均使用 FilterPolicy。这样的设计可以保证使用者可以自行定义策略类、方便地替换原有的布隆过滤器。这种设计也称之为策略模式，将策略单独设计为一个类或接口，不同的子类对应不同的策略方法。


BloomFilterPolicy 构造时需要提供bits_per_key，后再根据 key 的数量$n$ 一起计算出所需要的 bits 数$m$。而代码中的 k_ 即为布隆过滤器中哈希函数的数目$k$，这里 $k=m/n\times \ln2$

而后，一次计算$n$个key的k个哈希结果。这里使用了Double Hash，即

$h(key,i)=(h_1(key)+j\times h_2(key)) \mod Table\_Size$ 

Double Hash一般用于开放寻址哈希的优化，这里Table_Size为$2^{32}$，即uint32。
这里直接取连续的k个哈希结果作为布隆过滤器需要的k个哈希函数结果，一切为了速度。这里的$h_1$为代码中的BloomHash函数；而$h_2$为代码中的delta，也就是$h_1(key)$循环右移17位的结果。
Match函数则为逆过程，依次检查各个bit是否被标记即可。

参考链接：https://sf-zhou.github.io/leveldb/leveldb_02_data_structure.html

根据源码，LevelDB 在实现时，有以下几个点需要注意：
1. 之前提到的 bit 数组在 C++ 语言中，LevelDB 使用 char 数组来表示。因此计算 bit 数组长度时需要对齐到 8 的倍数，计算下标时需要进行适当转换。
2. LevelDB 实现时并未真正使用 k 个哈希函数，而是用了 double-hashing 方法进行了一个优化，号称可以达到相似的正确率。

# 布隆过滤器与std::set对比
1. 布隆过滤器比std::set占用的内存空间更小，因为布隆过滤器根本不存储数据
2. 布隆过滤器返回的查询结果可能是错的
