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


} // ns_log

#endif 