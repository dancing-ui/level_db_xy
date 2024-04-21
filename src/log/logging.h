#ifndef _LEVEL_DB_XY_LOGGING_H_
#define _LEVEL_DB_XY_LOGGING_H_

#include "slice.h"
#include <string>

namespace ns_log {

// Append a human-readable printout of "value" to *str.
// Escapes any non-printable characters found in "value".
void AppendEscapedStringTo(std::string* str, ns_data_structure::Slice const& value);

// Return a human-readable version of "value".
// Escapes any non-printable characters found in "value".
std::string EscapeString(ns_data_structure::Slice const& value); 

// Return a human-readable printout of "num"
std::string NumberToString(uint64_t num);

// Append a human-readable printout of "num" to *str
void AppendNumberTo(std::string* str, uint64_t num);

} // ns_log

#endif 