#ifndef _LEVEL_DB_XY_TWO_LEVEL_ITERATOR_H_
#define _LEVEL_DB_XY_TWO_LEVEL_ITERATOR_H_

#include "iterator.h"
#include "options.h"

namespace ns_table {

// Return a new two level iterator.  A two-level iterator contains an
// index iterator whose values point to a sequence of blocks where
// each block is itself a sequence of key,value pairs.  The returned
// two-level iterator yields the concatenation of all key/value pairs
// in the sequence of blocks.  Takes ownership of "index_iter" and
// will delete it when no longer needed.
//
// Uses a supplied function to convert an index_iter value into
// an iterator over the contents of the corresponding block.
ns_iterator::Iterator *NewTwoLevelIterator(ns_iterator::Iterator *index_iter,
                                           ns_iterator::Iterator *(*block_function)(void *arg, ns_options::ReadOptions const &options, ns_data_structure::Slice const &index_value),
                                           void *arg,
                                           ns_options::ReadOptions const &options);

} // ns_table

#endif