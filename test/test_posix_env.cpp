#include <gtest/gtest.h>

#include "log.h"
#include "env.h"
#include "env_posix_test_helper.h"
#include "test_util.h"

using namespace ns_env;
using namespace ns_util;
using namespace ns_data_structure;

#if defined(HAVE_O_CLOEXEC)

#endif // HAVE_O_CLOEXEC

static constexpr int32_t kReadOnlyFileLimit = 4;
static constexpr int32_t kMmapLimit = 4;

class EnvPosixTest : public testing::Test {
public:
    static void SetFileLimits(int32_t read_only_file_limit, int32_t mmap_limit) {
        EnvPosixTestHelper::SetReadOnlyFDLimit(read_only_file_limit);
        EnvPosixTestHelper::SetReadOnlyMMapLimit(mmap_limit);
    }

    EnvPosixTest() :
        env_(Env::Default()) {
    }

    Env *env_;
};

TEST_F(EnvPosixTest, TestOpenOnRead) {
    std::string test_dir = "/tmp/leveldbtest-1000";
    ASSERT_LEVELDB_OK(env_->GetTestDirectory(&test_dir));
    std::string test_file = test_dir + "/open_on_read.txt";

    FILE *f = std::fopen(test_file.c_str(), "we");
    ASSERT_TRUE(f != nullptr);
    char const kFileData[] = "abcdefghijklmnopqrstuvwxyz";
    fputs(kFileData, f);
    std::fclose(f);

    constexpr int32_t kNumFiles = kReadOnlyFileLimit + kMmapLimit + 5;
    RandomAccessFile *files[kNumFiles] = {0};
    for (int32_t i = 0; i < kNumFiles; i++) {
        ASSERT_LEVELDB_OK(env_->NewRandomAeccessFile(test_file, &files[i]));
    }

    char scratch;
    Slice read_result;
    for (int32_t i = 0; i < kNumFiles; i++) {
        ASSERT_LEVELDB_OK(files[i]->Read(i, 1, &read_result, &scratch));
        ASSERT_EQ(kFileData[i], read_result[0]);
    }
    for (int32_t i = 0; i < kNumFiles; i++) {
        delete files[i];
    }
    ASSERT_LEVELDB_OK(env_->RemoveFile(test_file));
}

int main(int argc, char **argv) {
    PRINT_INFO("Running main() from %s\n", __FILE__);
    EnvPosixTest::SetFileLimits(kReadOnlyFileLimit, kMmapLimit);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}