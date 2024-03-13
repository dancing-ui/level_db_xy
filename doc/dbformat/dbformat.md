# InternalKey
主键由3部分构成：User key + Sequence number + Value Type
1. User key是Slice
2. Sequence number是一个uint64_t类型（8字节无符号正整数）且一直递增的整数
3. Value Type是一个枚举值，具体什么作用还不清楚

主键统一存放在std::string中，Sequence number只会使用7个字节，Value Type占一个字节