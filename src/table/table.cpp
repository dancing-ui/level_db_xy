#include "table.h"
#include "filter_block.h"
#include "block.h"
#include "two_level_iterator.h"

namespace ns_table {

struct Table::Rep {
    ~Rep() {
        delete filter;
    }

    ns_options::Options options;
    ns_util::Status status;
    ns_env::RandomAccessFile *file;
    uint64_t cache_id;
    FilterBlockReader *filter;
    uint8_t const *filter_data;

    BlockHandle metaindex_handle; // Handle to metaindex_block: saved from footer
    Block *index_block;
};

ns_util::Status Table::Open(ns_options::Options const &options, ns_env::RandomAccessFile *file, uint64_t file_size, Table **table) {
    *table = nullptr;
    if (file_size < Footer::kEncodedLength) {
        return ns_util::Status::Corruption("file is too short to be an sstable");
    }

    char footer_space[Footer::kEncodedLength];
    ns_data_structure::Slice footer_input;
    ns_util::Status s = file->Read(file_size - Footer::kEncodedLength, Footer::kEncodedLength, &footer_input, footer_space);

    if (!s.ok()) {
        return s;
    }

    Footer footer;
    s = footer.DecodeFrom(&footer_input);
    if (!s.ok()) {
        return s;
    }

    // Read the index block
    BlockContents index_block_contents;
    ns_options::ReadOptions opt;
    if (options.paranoid_checks) {
        opt.verify_checksums = true;
    }
    s = ReadBlock(file, opt, footer.index_handle(), &index_block_contents);

    if (s.ok()) {
        Block *index_block = new Block(index_block_contents);
        Rep *rep = new Table::Rep;
        rep->options = options;
        rep->file = file;
        rep->metaindex_handle = footer.metaindex_handle();
        rep->index_block = index_block;
        rep->cache_id = (options.block_cache ? options.block_cache->NewId() : 0);
        rep->filter_data = nullptr;
        rep->filter = nullptr;
        *table = new Table(rep);
        (*table)->ReadMeta(footer);
    }

    return s;
}

Table::~Table() {
    delete rep_;
}

ns_iterator::Iterator *Table::NewIterator(ns_options::ReadOptions const &options) const {
    return NewTwoLevelIterator(rep_->index_block->NewIterator(rep_->options.comparator),
                               &Table::BlockReader, const_cast<Table *>(this), options);
}

uint64_t Table::ApproximateOffsetOf(ns_data_structure::Slice const &key) const {
    ns_iterator::Iterator *index_iter = rep_->index_block->NewIterator(rep_->options.comparator);
    index_iter->Seek(key);
    uint64_t result;
    if (index_iter->Valid()) {
        BlockHandle handle;
        ns_data_structure::Slice input = index_iter->value();
        ns_util::Status s = handle.DecodeFrom(&input);
        if (s.ok()) {
            result = handle.offset();
        } else {
            // Strange: we can't decode the block handle in the index block.
            // We'll just return the offset of the metaindex block, which is
            // close to the whole file size for this case.
            result = rep_->metaindex_handle.offset();
        }
    } else {
        // key is past the last key in the file.  Approximate the offset
        // by returning the offset of the metaindex block (which is
        // right near the end of the file).
        result = rep_->metaindex_handle.offset();
    }
    delete index_iter;
    return result;
}

static void DeleteBlock(void *arg, void *ignored) {
    delete reinterpret_cast<Block *>(arg);
}

static void ReleaseBlock(void *arg, void *h) {
    ns_cache::Cache *cache = reinterpret_cast<ns_cache::Cache *>(arg);
    ns_cache::Cache::Handle *handle = reinterpret_cast<ns_cache::Cache::Handle *>(h);
    cache->Release(handle);
}

static void DeleteCachedBlock(ns_data_structure::Slice const &key, void *value) {
    Block *block = reinterpret_cast<Block *>(value);
    delete block;
}

ns_iterator::Iterator *Table::BlockReader(void *arg, ns_options::ReadOptions const &options, ns_data_structure::Slice const &index_value) {
    Table *table = reinterpret_cast<Table *>(arg);
    ns_cache::Cache *block_cache = table->rep_->options.block_cache;
    Block *block = nullptr;
    ns_cache::Cache::Handle *cache_handle = nullptr;

    BlockHandle handle;
    ns_data_structure::Slice input = index_value;
    ns_util::Status s = handle.DecodeFrom(&input);

    if (s.ok()) {
        BlockContents contents;
        if (block_cache != nullptr) {
            uint8_t cache_key_buffer[16];
            ns_util::EncodeFixed64(cache_key_buffer, table->rep_->cache_id);
            ns_util::EncodeFixed64(cache_key_buffer + 8, handle.offset());
            ns_data_structure::Slice key{cache_key_buffer, sizeof(cache_key_buffer)};
            cache_handle = block_cache->Lookup(key);
            if (cache_handle != nullptr) {
                block = reinterpret_cast<Block *>(block_cache->Value(cache_handle));
            } else {
                s = ReadBlock(table->rep_->file, options, handle, &contents);
                if (s.ok()) {
                    block = new Block(contents);
                    if (contents.cachable && options.fill_cache) {
                        cache_handle = block_cache->Insert(key, block, block->size(), &DeleteCachedBlock);
                    }
                }
            }
        }
    }

    ns_iterator::Iterator *iter;
    if (block != nullptr) {
        iter = block->NewIterator(table->rep_->options.comparator);
        if (cache_handle == nullptr) {
            iter->RegisterCleanup(&DeleteBlock, block, nullptr);
        } else {
            iter->RegisterCleanup(&ReleaseBlock, block_cache, cache_handle);
        }
    } else {
        iter = ns_iterator::NewErrorIterator(s);
    }
    return iter;
}

ns_util::Status Table::InternalGet(ns_options::ReadOptions const &options, ns_data_structure::Slice const &key,
                                   void *arg, void (*handle_result)(void *arg, ns_data_structure::Slice const &k, ns_data_structure::Slice const &v)) {
    ns_util::Status s;
    ns_iterator::Iterator *iter = rep_->index_block->NewIterator(rep_->options.comparator);
    iter->Seek(key);
    if (iter->Valid()) {
        ns_data_structure::Slice handle_value = iter->value();
        FilterBlockReader *filter = rep_->filter;
        BlockHandle handle;
        if (filter != nullptr && handle.DecodeFrom(&handle_value).ok() && !filter->KeyMayMatch(handle.offset(), key)) {
            // Not Found
        } else {
            ns_iterator::Iterator *block_iter = BlockReader(this, options, iter->value());
            block_iter->Seek(key);
            if (block_iter->Valid()) {
                (*handle_result)(arg, block_iter->key(), block_iter->value());
            }
            s = block_iter->status();
            delete block_iter;
        }
    }
    if (s.ok()) {
        s = iter->status();
    }
    delete iter;
    return s;
}

void Table::ReadMeta(Footer const &footer) {
    if (rep_->options.filter_policy == nullptr) {
        return; // Do not need any metadata
    }

    ns_options::ReadOptions opt;
    if (rep_->options.paranoid_checks) {
        opt.verify_checksums = true;
    }
    BlockContents contents;
    if (!ReadBlock(rep_->file, opt, footer.metaindex_handle(), &contents).ok()) {
        // Do not propagate errors since meta info is not needed for operation
        return;
    }
    Block *meta = new Block(contents);

    ns_iterator::Iterator *iter = meta->NewIterator(ns_comparator::BytewiseComparator());
    std::string key = "filter.";
    key.append(rep_->options.filter_policy->Name());
    iter->Seek(key);
    if (iter->Valid() && iter->key() == ns_data_structure::Slice{key}) {
        ReadFilter(iter->value());
    }
    delete iter;
    delete meta;
}

void Table::ReadFilter(ns_data_structure::Slice const &filter_handle_value) {
    ns_data_structure::Slice v = filter_handle_value;
    BlockHandle filter_handle;
    if (!filter_handle.DecodeFrom(&v).ok()) {
        return;
    }

    ns_options::ReadOptions opt;
    if (rep_->options.paranoid_checks) {
        opt.verify_checksums = true;
    }
    BlockContents block;
    if (!ReadBlock(rep_->file, opt, filter_handle, &block).ok()) {
        return;
    }
    if (block.heap_allocated) {
        rep_->filter_data = block.data.data(); // Will need to delete later
    }
    rep_->filter = new FilterBlockReader(rep_->options.filter_policy, block.data);
}

} // ns_table