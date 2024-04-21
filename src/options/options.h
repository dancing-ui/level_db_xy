#ifndef _LEVEL_DB_XY_OPTIONS_H_
#define _LEVEL_DB_XY_OPTIONS_H_

#include "comparator.h"
#include "env.h"
#include "cache.h"
#include "filter_policy.h"

namespace ns_options {

struct Options {
    Options();

    ns_comparator::Comparator const *comparator;

    bool create_if_missing{false};

    bool error_if_exists{false};

    bool paranoid_checks{false};

    ns_env::Env *env;

    ns_env::Logger *info_log{nullptr};

    uint64_t write_buffer_size{4 * 1024 * 1024};

    int32_t max_open_files{1000};

    ns_cache::Cache *block_cache{nullptr};

    uint64_t block_size{4*1024};

    int32_t block_restart_interval{16};

    uint64_t max_file_size{2*1024*1024};

    //TODO: compression

    bool reuse_logs{false};

    ns_filter_policy::FilterPolicy const* filter_policy{nullptr};
};

} // ns_options

#endif