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

void AppendNumberTo(std::string* str, uint64_t num) {
    char buf[30];
    std::snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(num));
    str->append(buf);
}

std::string NumberToString(uint64_t num) {
    std::string r;
    AppendNumberTo(&r, num);
    return r;
}

} // ns_log