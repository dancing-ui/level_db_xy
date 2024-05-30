#include "table_builder.h"
#include "filter_block.h"
#include "crc32c.h"

namespace ns_table {

struct TableBuilder::Rep {
    Rep(ns_options::Options const &opt, ns_env::WritableFile *f) :
        options(opt), index_block_options(opt),
        file(f), offset(0), data_block(&options),
        index_block(&index_block_options),
        num_entries(0), closed(false),
        filter_block(opt.filter_policy == nullptr ? nullptr : new FilterBlockBuilder(opt.filter_policy)),
        pending_index_entry(false) {
        index_block_options.block_restart_interval = 1;
    }

    ns_options::Options options;
    ns_options::Options index_block_options;
    ns_env::WritableFile *file;
    uint64_t offset;
    ns_util::Status status;
    BlockBuilder data_block;
    BlockBuilder index_block;
    std::string last_key;
    int64_t num_entries;
    bool closed; // Either Finish() or Abandon() has been called.
    FilterBlockBuilder *filter_block;

    // We do not emit the index entry for a block until we have seen the
    // first key for the next data block.  This allows us to use shorter
    // keys in the index block.  For example, consider a block boundary
    // between the keys "the quick brown fox" and "the who".  We can use
    // "the r" as the key for the index block entry since it is >= all
    // entries in the first block and < all entries in subsequent
    // blocks.
    //
    // Invariant: r->pending_index_entry is true only if data_block is empty.
    bool pending_index_entry;
    BlockHandle pending_handle; // Handle to add to index block

    std::string compressed_output;
};

TableBuilder::TableBuilder(ns_options::Options const &options, ns_env::WritableFile *file) :
    rep_(new Rep(options, file)) {
    if (rep_->filter_block != nullptr) {
        rep_->filter_block->StartBlock(0);
    }
}

TableBuilder::~TableBuilder() {
    assert(rep_->closed); // Catch errors where caller forgot to call Finish()
    delete rep_->filter_block;
    delete rep_;
}

ns_util::Status TableBuilder::ChangeOptions(ns_options::Options const &options) {
    // Note: if more fields are added to Options, update
    // this function to catch changes that should not be allowed to
    // change in the middle of building a Table.
    if (options.comparator != rep_->options.comparator) {
        return ns_util::Status::InvalidArgument("changing comparator while building table");
    }
    // Note that any live BlockBuilders point to rep_->options and therefore
    // will automatically pick up the updated options.
    rep_->options = options;
    rep_->index_block_options = options;
    rep_->index_block_options.block_restart_interval = 1;
    return ns_util::Status::OK();
}

void TableBuilder::Add(ns_data_structure::Slice const &key, ns_data_structure::Slice const &value) {
    Rep *r = rep_;
    assert(!r->closed);
    if (!ok()) {
        return;
    }
    if (r->num_entries > 0) {
        assert(r->options.comparator->Compare(key, ns_data_structure::Slice(r->last_key)) > 0);
    }

    if (r->pending_index_entry) {
        assert(r->data_block.empty());
        r->options.comparator->FindShortestSeparator(&r->last_key, key);
        std::string handle_encoding;
        r->pending_handle.EncodeTo(&handle_encoding);
        r->index_block.Add(r->last_key, ns_data_structure::Slice(handle_encoding));
        r->pending_index_entry = false;
    }

    if (r->filter_block != nullptr) {
        r->filter_block->AddKey(key);
    }

    r->last_key.assign(reinterpret_cast<char const *>(key.data()), key.size());
    r->num_entries++;
    r->data_block.Add(key, value);

    uint64_t const estimated_block_size = r->data_block.CurrentSizeEstimate();
    if (estimated_block_size >= r->options.block_size) {
        Flush();
    }
}

void TableBuilder::Flush() {
    Rep *r = rep_;
    assert(!r->closed);
    if (!ok()) {
        return;
    }
    if (r->data_block.empty()) {
        return;
    }
    assert(!r->pending_index_entry);
    WriteBlock(&r->data_block, &r->pending_handle);
    if (ok()) {
        r->pending_index_entry = true;
        r->status = r->file->Flush();
    }
    if (r->filter_block != nullptr) {
        r->filter_block->StartBlock(r->offset);
    }
}

ns_util::Status TableBuilder::status() const {
    return rep_->status;
}

ns_util::Status TableBuilder::Finish() {
    Rep *r = rep_;
    Flush();
    assert(!r->closed);
    r->closed = true;

    BlockHandle filter_block_handle, metaindex_block_handle, index_block_handle;
    // Write filter block
    if (ok() && r->filter_block != nullptr) {
        WriteRawBlock(r->filter_block->Finish(), ns_compression::CompressionType::kNoCompression, &filter_block_handle);
    }

    // Write metaindex block
    if (ok()) {
        BlockBuilder meta_index_block(&r->options);
        if (r->filter_block != nullptr) {
            // Add mapping from "filter.Name" to location of filter data
            std::string key{"filter."};
            key.append(r->options.filter_policy->Name());
            std::string handle_encoding;
            filter_block_handle.EncodeTo(&handle_encoding);
            meta_index_block.Add(key, handle_encoding);
        }

        // TODO(postrelease): Add stats and other meta blocks
        WriteBlock(&meta_index_block, &metaindex_block_handle);
    }

    // Write index block
    if (ok()) {
        if (r->pending_index_entry) {
            r->options.comparator->FindShortSuccessor(&r->last_key);
            std::string handle_encoding;
            r->pending_handle.EncodeTo(&handle_encoding);
            r->index_block.Add(r->last_key, ns_data_structure::Slice(handle_encoding));
            r->pending_index_entry = false;
        }
        WriteBlock(&r->index_block, &index_block_handle);
    }

    // Write footer
    if (ok()) {
        Footer footer;
        footer.set_metaindex_handle(metaindex_block_handle);
        footer.set_index_handle(index_block_handle);
        std::string footer_encoding;
        footer.EncodeTo(&footer_encoding);
        r->status = r->file->Append(footer_encoding);
        if (r->status.ok()) {
            r->offset += footer_encoding.size();
        }
    }
    return r->status;
}

void TableBuilder::Abandon() {
    Rep *r = rep_;
    assert(!r->closed);
    r->closed = true;
}

uint64_t TableBuilder::NumEntries() const {
    return rep_->num_entries;
}

uint64_t TableBuilder::FileSize() const {
    return rep_->offset;
}

void TableBuilder::WriteBlock(BlockBuilder *block, BlockHandle *handle) {
    // File format contains a sequence of blocks where each block has:
    //    block_data: uint8[n]
    //    type: uint8
    //    crc: uint32
    assert(ok());
    Rep *r = rep_;
    ns_data_structure::Slice raw = block->Finish();

    ns_data_structure::Slice block_contents;
    ns_compression::CompressionType type = r->options.compression;
    // TODO(postrelease): Support more compression options: zlib?
    switch (type) {
    case ns_compression::CompressionType::kNoCompression: {
        block_contents = raw;
        break;
    }
    case ns_compression::CompressionType::kSnappyCompression: {
        std::string *compressed = &r->compressed_output;
        if (ns_compression::Snappy_Compress(reinterpret_cast<char const *>(raw.data()), raw.size(), compressed) && compressed->size() < raw.size() - (raw.size() / 8U)) {
            block_contents = *compressed;
        } else {
            // Snappy not supported, or compressed less than 12.5%, so just
            // store uncompressed form
            block_contents = raw;
            type = ns_compression::CompressionType::kNoCompression;
        }
        break;
    }
    case ns_compression::CompressionType::kZstdCompression: {
        std::string *compressed = &r->compressed_output;
        if (ns_compression::Zstd_Compress(r->options.zstd_compression_level, reinterpret_cast<char const *>(raw.data()), raw.size(), compressed) && compressed->size() < raw.size() - (raw.size() / 8U)) {
            block_contents = *compressed;
        } else {
            // Zstd not supported, or compressed less than 12.5%, so just
            // store uncompressed form
            block_contents = raw;
            type = ns_compression::CompressionType::kNoCompression;
        }
        break;
    }
    default: {
        break;
    }
    }
    WriteRawBlock(block_contents, type, handle);
    r->compressed_output.clear();
    block->Reset();
}

void TableBuilder::WriteRawBlock(ns_data_structure::Slice const &block_contents, ns_compression::CompressionType const &type, BlockHandle *handle) {
    Rep *r = rep_;
    handle->set_offset(r->offset);
    handle->set_size(block_contents.size());
    r->status = r->file->Append(block_contents);
    if (r->status.ok()) {
        uint8_t trailer[kBlockTrailerSize];
        trailer[0] = type;
        uint32_t crc = ns_util::Value(block_contents.data(), block_contents.size());
        crc = ns_util::Extend(crc, trailer, 1);
        ns_util::EncodeFixed32(trailer + 1, ns_util::Mask(crc));
        r->status = r->file->Append(ns_data_structure::Slice(trailer, kBlockTrailerSize));
        if (r->status.ok()) {
            r->offset += block_contents.size() + kBlockTrailerSize;
        }
    }
}

} // ns_table