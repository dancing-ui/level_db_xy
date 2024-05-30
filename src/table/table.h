#ifndef _LEVEL_DB_XY_TABLE_H_
#define _LEVEL_DB_XY_TABLE_H_

#include "iterator.h"
#include "format.h"

namespace ns_table {
// A Table is a sorted map from strings to strings.  Tables are
// immutable and persistent.  A Table may be safely accessed from
// multiple threads without external synchronization.
class Table {
public:
    static ns_util::Status Open(ns_options::Options const &options, ns_env::RandomAccessFile *file, uint64_t file_size, Table **table);

    Table(Table const &) = delete;
    Table &operator=(Table const &) = delete;

    ~Table();

    ns_iterator::Iterator *NewIterator(ns_options::ReadOptions const &options) const;

    uint64_t ApproximateOffsetOf(ns_data_structure::Slice const &key) const;

private:
    friend class TableCache;

    struct Rep;

    static ns_iterator::Iterator *BlockReader(void *arg, ns_options::ReadOptions const &options, ns_data_structure::Slice const &index_value);

    explicit Table(Rep *rep) :
        rep_(rep) {
    }

    ns_util::Status InternalGet(ns_options::ReadOptions const &options, ns_data_structure::Slice const &key,
                                void *arg, void (*handle_result)(void *arg, ns_data_structure::Slice const &k, ns_data_structure::Slice const &v));

    void ReadMeta(Footer const &footer);

    void ReadFilter(ns_data_structure::Slice const &filter_handle_value);

    Rep *const rep_;
};

} // ns_table

#endif