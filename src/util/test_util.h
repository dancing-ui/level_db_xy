#ifndef _LEVEL_DB_XY_TEST_UTIL_H_
#define _LEVEL_DB_XY_TEST_UTIL_H_

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "env.h"

namespace ns_util {

MATCHER(IsOK, "") { return arg.ok(); }

#define ASSERT_LEVELDB_OK(expression) \
    ASSERT_THAT(expression, IsOK())

} // ns_util

#endif