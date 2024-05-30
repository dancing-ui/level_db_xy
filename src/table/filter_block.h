#ifndef _LEVEL_DB_XY_FILTER_BLOCK_H_
#define _LEVEL_DB_XY_FILTER_BLOCK_H_

#include "filter_policy.h"

#include <vector>

namespace ns_table {

class FilterBlockBuilder {
public:
    explicit FilterBlockBuilder(ns_filter_policy::FilterPolicy const *);

    FilterBlockBuilder(FilterBlockBuilder const &) = delete;
    FilterBlockBuilder &operator=(FilterBlockBuilder const &) = delete;

    void StartBlock(uint64_t block_offset);
    void AddKey(ns_data_structure::Slice const &key);
    ns_data_structure::Slice Finish();

private:
    void GenerateFilter();

    ns_filter_policy::FilterPolicy const *policy_;
    std::string keys_;                              // Flattened key contents
    std::vector<uint64_t> start_;                   // Starting index in keys_ of each key
    std::string result_;                            // Filter data computed so far
    std::vector<ns_data_structure::Slice> tmp_keys_; // policy_->CreateFilter() argument
    std::vector<uint32_t> filter_offsets_;
};

class FilterBlockReader {
public:
    FilterBlockReader(ns_filter_policy::FilterPolicy const* policy, ns_data_structure::Slice const& contents);
    bool KeyMayMatch(uint64_t block_offset, ns_data_structure::Slice const& key);

private:
    ns_filter_policy::FilterPolicy const* policy_;
    uint8_t const* data_;
    uint8_t const* offset_;
    uint64_t num_;
    uint64_t base_lg_;
};

} // ns_table

#endif