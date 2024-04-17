#ifndef _LEVEL_DB_XY_ENV_POSIX_TEST_HELPER_H_
#define _LEVEL_DB_XY_ENV_POSIX_TEST_HELPER_H_

#include <cstdint>

class EnvPosixTest;

namespace ns_env {

class EnvPosixTestHelper {
public:
    friend class EnvPosixTest;

    static void SetReadOnlyFDLimit(int32_t limit);

    static void SetReadOnlyMMapLimit(int32_t limit);
};

} // ns_env

#endif