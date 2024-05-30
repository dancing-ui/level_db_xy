#ifndef _LEVEL_DB_XY_TABLE_BUILDER_H_
#define _LEVEL_DB_XY_TABLE_BUILDER_H_

#include "options.h"
#include "env.h"
#include "status.h"
#include "block_builder.h"
#include "format.h"
#include "compression.h"

namespace ns_table {

class TableBuilder {
public:
    // Create a builder that will store the contents of the table it is
    // building in *file.  Does not close the file.  It is up to the
    // caller to close the file after calling Finish().
    TableBuilder(ns_options::Options const &options, ns_env::WritableFile *file);

    TableBuilder(TableBuilder const &) = delete;
    TableBuilder &operator=(TableBuilder const &) = delete;

    // REQUIRES: Either Finish() or Abandon() has been called.
    ~TableBuilder();

    // Change the options used by this builder.  Note: only some of the
    // option fields can be changed after construction.  If a field is
    // not allowed to change dynamically and its value in the structure
    // passed to the constructor is different from its value in the
    // structure passed to this method, this method will return an error
    // without changing any fields.
    ns_util::Status ChangeOptions(ns_options::Options const &options);

    // Add key,value to the table being constructed.
    // REQUIRES: key is after any previously added key according to comparator.
    // REQUIRES: Finish(), Abandon() have not been called
    void Add(ns_data_structure::Slice const &key, ns_data_structure::Slice const &value);

    // Advanced operation: flush any buffered key/value pairs to file.
    // Can be used to ensure that two adjacent entries never live in
    // the same data block.  Most clients should not need to use this method.
    // REQUIRES: Finish(), Abandon() have not been called
    void Flush();

    // Return non-ok iff some error has been detected.
    ns_util::Status status() const;

    // Finish building the table.  Stops using the file passed to the
    // constructor after this function returns.
    // REQUIRES: Finish(), Abandon() have not been called
    ns_util::Status Finish();

    // Indicate that the contents of this builder should be abandoned.  Stops
    // using the file passed to the constructor after this function returns.
    // If the caller is not going to call Finish(), it must call Abandon()
    // before destroying this builder.
    // REQUIRES: Finish(), Abandon() have not been called
    void Abandon();

    // Number of calls to Add() so far.
    uint64_t NumEntries() const;

    // Size of the file generated so far.  If invoked after a successful
    // Finish() call, returns the size of the final generated file.
    uint64_t FileSize() const;

private:
    bool ok() const {
        return status().ok();
    }

    void WriteBlock(BlockBuilder *block, BlockHandle *handle);

    void WriteRawBlock(ns_data_structure::Slice const &block_contents, ns_compression::CompressionType const &type, BlockHandle *handle);

    struct Rep;

    Rep *rep_;
};

} // ns_table

#endif