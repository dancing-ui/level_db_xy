#include "filename.h"
#include <cassert>
#include <cstdio>

namespace ns_util {

static std::string MakeFileName(std::string const &dbname, uint64_t number, char const *suffix) {
    char buf[100];
    std::snprintf(buf, sizeof(buf), "/%06llu.%s", static_cast<unsigned long long>(number), suffix);
    return dbname + buf;
}

std::string TableFileName(std::string const &dbname, uint64_t number) {
    assert(number > 0);
    return MakeFileName(dbname, number, "ldb");
}

std::string SSTTableFileName(std::string const& dbname, uint64_t number) {
    assert(number > 0);
    return MakeFileName(dbname, number, "sst");
}

} // ns_util