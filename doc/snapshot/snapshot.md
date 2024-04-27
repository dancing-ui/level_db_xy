# 实现双向链表非常优雅的方式
```C++

class SnapshotList;
// 链表节点
class SnapshotImpl : public Snapshot {
public:
    SnapshotImpl(ns_db_format::SequenceNumber sequence_number) :
        sequence_number_(sequence_number) {
    }

private:
    friend class SnapshotList;
    // SnapshotImpl is kept in a doubly-linked circular list. The SnapshotList
    // implementation operates on the next/previous fields directly.
    SnapshotImpl *prev_;
    SnapshotImpl *next_;

    ns_db_format::SequenceNumber sequence_number_;
#if !defined(NDEBUG)
    SnapshotList *list_{nullptr}; // 保存当前节点所属的链表对象
#endif // NDEBUG
};
// 链表类管理链表中的节点
class SnapshotList {
public:
    SnapshotList() :
        head_(0) {
        // 初始指向自己
        head_.prev_ = &head_; 
        head_.next_ = &head_;
    }

    bool empty() const {
        return head_.next_ == &head_;
    }

    SnapshotImpl *oldest() const {
        assert(!empty()); // 细节判空
        return head_.next_;
    }

    SnapshotImpl *newest() const {
        assert(!empty());// 细节判空
        return head_.prev_;
    }

    SnapshotImpl *New(ns_db_format::SequenceNumber sequence_number) {
        assert(empty() || newest()->sequence_number_ <= sequence_number);
        SnapshotImpl *snapshot = new SnapshotImpl(sequence_number);
        // 保存链表对象
#if !defined(NDEBUG)
        snapshot->list_ = this;
#endif // NDEBUG
        // 插入到头部，先修改自己，再修改别人
        snapshot->next_ = &head_;
        snapshot->prev_ = head_.prev_;
        snapshot->prev_->next_ = snapshot;
        snapshot->next_->prev_ = snapshot;
        return snapshot;
    }

    void Delete(SnapshotImpl const *snapshot) {
#if !defined(NDEBUG)
        assert(snapshot->list_ == this);
#endif
        // 断开自己，再delete
        snapshot->prev_->next_ = snapshot->next_;
        snapshot->next_->prev_ = snapshot->prev_;
        delete snapshot;
    }

private:
    // Dummy head of doubly-linked list of snapshots
    SnapshotImpl head_; // 只需要一个虚拟头节点
};
```