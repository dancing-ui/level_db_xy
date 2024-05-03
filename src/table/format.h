#ifndef _LEVEL_DB_XY_FORMAT_H_
#define _LEVEL_DB_XY_FORMAT_H_

#include "slice.h"

namespace ns_table {

struct BlockContents {
    ns_data_structure::Slice data; // Actual contents of data
    bool cachable;                 // True iff data can be cached
    bool heap_allocated;           // True iff caller should delete[] data.data()
};

} // ns_table

#endif