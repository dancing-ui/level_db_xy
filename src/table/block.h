#ifndef _LEVEL_DB_XY_BLOCK_H_
#define _LEVEL_DB_XY_BLOCK_H_

#include "format.h"
#include "iterator.h"
#include "comparator.h"

namespace ns_table {

class Block {
public:
    explicit Block(BlockContents const &contents);

    Block(Block const &) = delete;
    Block &operator=(Block const &) = delete;

    ~Block();

    uint64_t size() const {
        return size_;
    }

    ns_iterator::Iterator *NewIterator(ns_comparator::Comparator const *comparator);

private:
    class Iter;

    uint32_t NumRestarts() const;

    uint8_t const *data_;
    uint32_t size_;
    uint32_t restart_offset_; // Offset in data_ of restart array
    bool owned_;              // Block owns data_[]
};

} // ns_table

#endif