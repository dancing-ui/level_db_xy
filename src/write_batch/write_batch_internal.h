#ifndef _LEVEL_DB_XY_WRITE_BATCH_INTERNAL_H_
#define _LEVEL_DB_XY_WRITE_BATCH_INTERNAL_H_

#include "db_format.h"
#include "write_batch.h"
#include "mem_table.h"

namespace ns_write_batch {

class WriteBatchInternal {
public:
    // Return the number of entries in the batch.
    static int32_t Count(WriteBatch const *batch);
    // Set the count for the number of entries in the batch.
    static void SetCount(WriteBatch *batch, int32_t n);
    // Return the sequence number for the start of this batch.
    static ns_db_format::SequenceNumber Sequence(WriteBatch const *batch);
    // Store the specified number as the sequence number for the start of
    // this batch.
    static void SetSequence(WriteBatch *batch, ns_db_format::SequenceNumber seq);
    static ns_data_structure::Slice Contents(WriteBatch const *batch) {
        return ns_data_structure::Slice(batch->rep_);
    }
    static uint64_t ByteSize(WriteBatch const *batch) {
        return batch->rep_.size();
    }
    static void SetContents(WriteBatch *batch, ns_data_structure::Slice const& contents);
    static ns_util::Status InsertInto(WriteBatch const *batch, ns_data_structure::MemTable *memtable);
    static void Append(WriteBatch *dst, WriteBatch const *src);
};

} // ns_write_batch

#endif