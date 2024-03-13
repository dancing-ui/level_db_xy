#include "logging.h"


namespace ns_log {

void AppendEscapedStringTo(std::string* str, ns_data_structure::Slice const& value) {
    for(uint64_t i=0;i<value.size();i++) {
        uint8_t c = value[i];
        if(c >= ' ' && c<='~') {
            str->push_back(c);
        } else {
            char buf[10];
            std::snprintf(buf, sizeof(buf), "\\x%02x", c & 0xFFU);
            str->append(buf);
        }
    }
}

std::string EscapeString(ns_data_structure::Slice const& value) {
    std::string ret;
    AppendEscapedStringTo(&ret, value);
    return ret;
}


} // ns_log