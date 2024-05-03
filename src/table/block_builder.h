#ifndef _LEVEL_DB_XY_BLOCK_BUILDER_H_
#define _LEVEL_DB_XY_BLOCK_BUILDER_H_

#include "options.h"

namespace ns_table {

class BlockBuilder {
public:
    explicit BlockBuilder(ns_options::Options const *options);

    BlockBuilder(BlockBuilder const &) = delete;
    BlockBuilder &operator=(BlockBuilder const &) = delete;

    void Reset();

    void Add(ns_data_structure::Slice const &key, ns_data_structure::Slice const &value);

    ns_data_structure::Slice Finish();

    uint64_t CurrentSizeEstimate() const;

    bool empty() const {
        return buffer_.empty();
    }

private:
    ns_options::Options const *options_;
    std::string buffer_;
    std::vector<uint32_t> restarts_;
    int32_t counter_;
    bool finished_;
    std::string last_key_;
};

} // ns_table

#endif