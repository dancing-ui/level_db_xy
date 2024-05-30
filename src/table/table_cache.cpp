#include "table_cache.h"
#include "filename.h"

namespace ns_table {

struct TableAndFile {
    ns_env::RandomAccessFile *file;
    Table *table;
};

static void DeleteEntry(ns_data_structure::Slice const &key, void *value) {
    TableAndFile *tf = reinterpret_cast<TableAndFile *>(value);
    delete tf->table;
    delete tf->file;
    delete tf;
}

static void UnrefEntry(void *arg1, void *arg2) {
    ns_cache::Cache *cache = reinterpret_cast<ns_cache::Cache *>(arg1);
    ns_cache::Cache::Handle *h = reinterpret_cast<ns_cache::Cache::Handle *>(arg2);
    cache->Release(h);
}

TableCache::TableCache(std::string const &dbname, ns_options::Options const &options, int32_t entries) :
    env_(options.env), dbname_(dbname),
    options_(options), cache_(ns_cache::NewLRUCache(entries)) {
}

TableCache::~TableCache() {
    delete cache_;
}

ns_iterator::Iterator *TableCache::NewIterator(ns_options::ReadOptions const &options, uint64_t file_number, uint64_t file_size, Table **tableptr) {
    if (tableptr != nullptr) {
        *tableptr = nullptr;
    }

    ns_cache::Cache::Handle *handle = nullptr;
    ns_util::Status s = FindTable(file_number, file_size, &handle);
    if (!s.ok()) {
        return ns_iterator::NewErrorIterator(s);
    }

    Table *table = reinterpret_cast<TableAndFile *>(cache_->Value(handle))->table;
    ns_iterator::Iterator *result = table->NewIterator(options);
    result->RegisterCleanup(&UnrefEntry, cache_, handle);
    if (tableptr != nullptr) {
        *tableptr = table;
    }
    return result;
}

ns_util::Status TableCache::Get(ns_options::ReadOptions const &options, uint64_t file_number, uint64_t file_size, ns_data_structure::Slice const &k, void *arg, void (*handle_result)(void *, ns_data_structure::Slice const &, ns_data_structure::Slice const &)) {
    ns_cache::Cache::Handle *handle = nullptr;
    ns_util::Status s = FindTable(file_number, file_size, &handle);
    if (s.ok()) {
        Table *t = reinterpret_cast<TableAndFile *>(cache_->Value(handle))->table;
        s = t->InternalGet(options, k, arg, handle_result);
        cache_->Release(handle);
    }
    return s;
}

void TableCache::Evict(uint64_t file_number) {
    uint8_t buf[sizeof(file_number)];
    ns_util::EncodeFixed64(buf, file_number);
    cache_->Erase(ns_data_structure::Slice(buf, sizeof(buf)));
}

ns_util::Status TableCache::FindTable(uint64_t file_number, uint64_t file_size, ns_cache::Cache::Handle **handle) {
    ns_util::Status s;
    uint8_t buf[sizeof(file_number)];
    ns_util::EncodeFixed64(buf, file_number);
    ns_data_structure::Slice key{buf, sizeof(buf)};
    *handle = cache_->Lookup(key);
    if (*handle == nullptr) {
        std::string fname = ns_util::TableFileName(dbname_, file_number);
        ns_env::RandomAccessFile *file = nullptr;
        Table *table = nullptr;
        s = env_->NewRandomAeccessFile(fname, &file);
        if (!s.ok()) {
            std::string old_fname = ns_util::SSTTableFileName(dbname_, file_number);
            if (env_->NewRandomAeccessFile(old_fname, &file).ok()) {
                s = ns_util::Status::OK();
            }
        }
        if (s.ok()) {
            s = Table::Open(options_, file, file_size, &table);
        }

        if (!s.ok()) {
            assert(table == nullptr);
            delete file;
        } else {
            TableAndFile *tf = new TableAndFile();
            tf->file = file;
            tf->table = table;
            *handle = cache_->Insert(key, tf, 1, &DeleteEntry);
        }
    }
    return s;
}

} // ns_table