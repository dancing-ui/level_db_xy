#ifndef _LEVEL_DB_XY_WRITE_BATCH_H_
#define _LEVEL_DB_XY_WRITE_BATCH_H_

#include "status.h"

namespace ns_write_batch {

class WriteBatch {
public:
    class Handler {
    public:
        virtual ~Handler() = default;
        virtual void Put(ns_data_structure::Slice const &key, ns_data_structure::Slice const &value) = 0;
        virtual void Delete(ns_data_structure::Slice const &key) = 0;
    };

    WriteBatch();
    WriteBatch(WriteBatch const &) = default;
    WriteBatch &operator=(WriteBatch const &) = default;

    ~WriteBatch() = default;

    void Put(ns_data_structure::Slice const &key, ns_data_structure::Slice const &value);
    void Delete(ns_data_structure::Slice const &key);
    void Clear();
    uint64_t ApproximateSize() const;
    void Append(WriteBatch const &source);
    ns_util::Status Iterate(Handler *handler) const;

private:
    friend class WriteBatchInternal;
    // WriteBatch::rep_ :=
    //    sequence: fixed64
    //    count: fixed32
    //    data: record[count]
    // record :=
    //    kTypeValue varstring varstring         |
    //    kTypeDeletion varstring
    // varstring :=
    //    len: varint32
    //    data: uint8[len]
    std::string rep_;
};

} // ns_write_batch

#endif