Block类用于解析每个数据项entry

Block中每个数据项entry的格式如下图所示：
![alt text](../../img/block_item.png)
1个entry分为5部分内容：
1. 与前一条记录key共享部分的长度；
2. 与前一条记录key不共享部分的长度；
3. value长度；
4. 与前一条记录key非共享的内容；
5. value内容；
# Restart Point
## 好处1
leveldb设计Restart point的目的是在读取sstable内容时，加速查找的过程。

由于每个Restart point存储的都是完整的key值，因此在sstable中进行数据查找时，可以首先利用restart point点的数据进行键值比较，以便于快速定位目标数据所在的区域；

当确定目标数据所在区域时，再依次对区间内所有数据项逐项比较key值，进行细粒度地查找；

该思想有点类似于跳表中利用高层数据迅速定位，底层数据详细查找的理念，降低查找的复杂度。
## 好处2
如果最开头的记录数据损坏，其后的所有记录都将无法恢复。为了降低这个风险，leveldb引入了重启点，每隔固定条数记录会强制加入一个重启点，这个位置的Entry会完整的记录自己的Key。

# 参考博客
https://blog.csdn.net/caoshangpa/article/details/78977743

https://leveldb-handbook.readthedocs.io/zh/latest/sstable.html