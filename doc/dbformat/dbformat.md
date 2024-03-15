# InternalKey
主键由3部分构成：User key + Sequence number + Value Type
1. User key是Slice
2. Sequence number是一个uint64_t类型（8字节无符号正整数）且一直递增的整数
3. Value Type是一个枚举值，具体什么作用还不清楚

键码统一存放在std::string中，Sequence number只会使用7个字节，Value Type占1个字节
# FindShortestSeparator
FindShortestSeparator将start更改为一个位于[start, limit)里的最短的字符串。这主要是为了优化SSTable里的Index Block里的索引项的长度，使得索引更短。因为每一个Data Block对应的索引项大于等于这个Data Block的最后一个项，而小于下一个Data Block的第一个项，通过这个函数可以减小索引项的长度；

# FindShortSuccessor
FindShortSuccessor将key更改为大于key的最短的key，这也是为了减小索引项的长度，不过这是优化一个SSTable里最后一个索引项的。
