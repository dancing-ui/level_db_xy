#ifndef _LEVEL_DB_XY_TABLE_CACHE_H_
#define _LEVEL_DB_XY_TABLE_CACHE_H_

#include "db_format.h"
#include "options.h"
#include "iterator.h"
#include "table.h"

namespace ns_table {

class TableCache {
public:
    TableCache(std::string const &dbname, ns_options::Options const &options, int32_t entries);

    TableCache(TableCache const &) = delete;
    TableCache &operator=(TableCache const &) = delete;

    ~TableCache();
    // Return an iterator for the specified file number (the corresponding
    // file length must be exactly "file_size" bytes).  If "tableptr" is
    // non-null, also sets "*tableptr" to point to the Table object
    // underlying the returned iterator, or to nullptr if no Table object
    // underlies the returned iterator.  The returned "*tableptr" object is owned
    // by the cache and should not be deleted, and is valid for as long as the
    // returned iterator is live.
    ns_iterator::Iterator *NewIterator(ns_options::ReadOptions const &options, uint64_t file_number, uint64_t file_size, Table **tableptr = nullptr);
    // If a seek to internal key "k" in specified file finds an entry,
    // call (*handle_result)(arg, found_key, found_value).
    ns_util::Status Get(ns_options::ReadOptions const &options, uint64_t file_number, uint64_t file_size, ns_data_structure::Slice const &k, void *arg, void (*handle_result)(void *, ns_data_structure::Slice const &, ns_data_structure::Slice const &));
    // Evict any entry for the specified file number
    void Evict(uint64_t file_number);

private:
    ns_util::Status FindTable(uint64_t file_number, uint64_t file_size, ns_cache::Cache::Handle **);

    ns_env::Env *const env_;
    std::string const dbname_;
    ns_options::Options const &options_;
    ns_cache::Cache *cache_;
};

} // na_table

#endif