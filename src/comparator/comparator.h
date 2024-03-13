#ifndef _LEVEL_DB_XY_COMPARATOR_H_
#define _LEVEL_DB_XY_COMPARATOR_H_

#include "slice.h"
#include <cstdint>

namespace ns_comparator {

class Comparator {
public:
    virtual ~Comparator() = default;
    virtual int32_t Compare(ns_data_structure::Slice const& a, ns_data_structure::Slice const& b) const = 0;
    virtual uint8_t const* Name() const = 0;
    virtual void FindShortestSeparator(std::string* start, ns_data_structure::Slice const& limit) const = 0;
    virtual void FindShortSuccessor(std::string* key) const = 0;
};

Comparator const* BytewiseComparator();

} // ns_comparator


#endif 