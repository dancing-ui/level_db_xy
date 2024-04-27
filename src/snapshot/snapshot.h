#ifndef _LEVEL_DB_XY_SNAPSHOT_H_
#define _LEVEL_DB_XY_SNAPSHOT_H_

#include "db_format.h"

namespace ns_snapshot {

class Snapshot {
protected:
    virtual ~Snapshot();
};

class SnapshotList;
// Snapshots are kept in a doubly-linked list in the DB.
// Each SnapshotImpl corresponds to a particular sequence number.
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
    SnapshotList *list_{nullptr};
#endif // NDEBUG
};

class SnapshotList {
public:
    SnapshotList() :
        head_(0) {
        head_.prev_ = &head_;
        head_.next_ = &head_;
    }

    bool empty() const {
        return head_.next_ == &head_;
    }

    SnapshotImpl *oldest() const {
        assert(!empty());
        return head_.next_;
    }

    SnapshotImpl *newest() const {
        assert(!empty());
        return head_.prev_;
    }

    SnapshotImpl *New(ns_db_format::SequenceNumber sequence_number) {
        assert(empty() || newest()->sequence_number_ <= sequence_number);
        SnapshotImpl *snapshot = new SnapshotImpl(sequence_number);

#if !defined(NDEBUG)
        snapshot->list_ = this;
#endif // NDEBUG
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
        snapshot->prev_->next_ = snapshot->next_;
        snapshot->next_->prev_ = snapshot->prev_;
        delete snapshot;
    }

private:
    // Dummy head of doubly-linked list of snapshots
    SnapshotImpl head_;
};

} // ns_snapshot

#endif