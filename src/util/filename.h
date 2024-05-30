#ifndef _LEVEL_DB_XY_FILENAME_H_
#define _LEVEL_DB_XY_FILENAME_H_

#include <string>

namespace ns_util {

std::string TableFileName(std::string const& dbname, uint64_t number);

std::string SSTTableFileName(std::string const& dbname, uint64_t number);

} // ns_util

#endif