#ifndef _LEVEL_DB_XY_MEM_TABLE_H_
#define _LEVEL_DB_XY_MEM_TABLE_H_

#include "db_format.h"
#include "iterator.h"
#include "skip_list.h"

namespace ns_data_structure {

class MemTable {
public:
    explicit MemTable(ns_db_format::InternalKeyComparator const &comparator);

    MemTable(MemTable const &) = delete;
    MemTable &operator=(MemTable const &) = delete;

    // Increase reference count.
    void Ref() {
        refs_++;
    }

    // Drop reference count.  Delete if no more references exist.
    void Unref() {
        refs_--;
        assert(refs_ >= 0);
        if (refs_ <= 0) {
            delete this;
        }
    }

    uint64_t ApproximateMemoryUsage();

    ns_iterator::Iterator *NewIterator();

    void Add(ns_db_format::SequenceNumber seq, ns_db_format::ValueType type, ns_data_structure::Slice const &key, ns_data_structure::Slice const &value);
    bool Get(ns_db_format::LookupKey const &key, std::string *value, ns_util::Status *s);

private:
    friend class MemTableIterator;
    friend class MemTableBackwardIterator;

    struct KeyComparator {
        ns_db_format::InternalKeyComparator const comparator;
        explicit KeyComparator(ns_db_format::InternalKeyComparator const &c) :
            comparator(c) {
        }
        int32_t operator()(uint8_t const *ap, uint8_t const *bp) const;
    };

    using Table = ns_data_structure::SkipList<uint8_t const *, KeyComparator>;

    ~MemTable() { assert(refs_ == 0); }; // Private since only Unref() should be used to delete it

    KeyComparator comparator_;
    int32_t refs_;
    ns_memory::Arena arena_;
    Table table_;
};

} // ns_data_structure

#endif