#include "env.h"
#include "thread_annotation.h"
#include "posix_logger.h"
#include "env_posix_test_helper.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/resource.h>

#include <atomic>
#include <mutex>
#include <set>
#include <thread>
#include <condition_variable>
#include <queue>
#include <assert.h>

namespace ns_env {

namespace {
// Common flags defined for all posix open operations
#if defined(HAVE_O_CLOEXEC)
constexpr int32_t kOpenBaseFlags = O_CLOEXEC;
#else
constexpr int32_t kOpenBaseFlags = 0;
#endif // HAVE_O_CLOEXEC

constexpr uint64_t kWritableFileBufferSize = 65536;
// Up to 1000 mmap regions for 64-bit binaries; none for 32-bit.
constexpr int32_t kDefaultMmapLimit = (sizeof(void *) >= 8) ? 1000 : 0;

int32_t g_mmap_limit = kDefaultMmapLimit;
// Set by EnvPosixTestHelper::SetReadOnlyMMapLimit() and MaxOpenFiles().
int32_t g_open_read_only_file_limit = -1;

ns_util::Status PosixError(std::string const &context, int32_t error_number) {
    if (error_number == ENONET) {
        return ns_util::Status::NotFound(context, std::strerror(error_number));
    }
    return ns_util::Status::IOError(context, std::strerror(error_number));
}

// Helper class to limit resource usage to avoid exhaustion.
// Currently used to limit read-only file descriptors and mmap file usage
// so that we do not run out of file descriptors or virtual memory, or run into
// kernel performance problems for very large databases.
class Limiter {
public:
    Limiter(int32_t max_acquires) :
#if !defined(NDEBUG)
        max_acquires_(max_acquires),
#endif // !defined(NDEBUG)
        acquires_allowed_(max_acquires) {
        assert(max_acquires >= 0);
    }

    Limiter(Limiter const &) = delete;
    Limiter &operator=(Limiter const &) = delete;
    // If another resource is available, acquire it and return true.
    // Else return false.
    bool Acquire() {
        // the fetch_sub return value is the value before sub
        int32_t old_acquires_allowed = acquires_allowed_.fetch_sub(1, std::memory_order_relaxed);
        if (old_acquires_allowed > 0) {
            return true;
        }
        // when (old_acquires_allowed <= 0), that means no have resource
        int32_t pre_increment_acquires_allowed = acquires_allowed_.fetch_add(1, std::memory_order_relaxed);
        static_cast<void>(pre_increment_acquires_allowed); // Silence compiler warnings about unused arguments when NDEBUG is defined.
        assert(pre_increment_acquires_allowed < max_acquires_);
        return false;
    }
    // Release a resource acquired by a previous call to Acquire() that returned
    // true.
    void Release() {
        int32_t old_acquires_allowed = acquires_allowed_.fetch_add(1, std::memory_order_relaxed);
        static_cast<void>(old_acquires_allowed);
        assert(old_acquires_allowed < max_acquires_);
    }

private:
#if !defined(NDEBUG)
    int32_t const max_acquires_{0};
#endif // !defined(NDEBUG)
    std::atomic<int32_t> acquires_allowed_{0};
};

class PosixSequentialFile final : public SequentialFile {
public:
    PosixSequentialFile(std::string filename, int32_t fd) :
        fd_(fd), filename_(std::move(filename)) {
    }

    ~PosixSequentialFile() override {
        close(fd_);
    }

    ns_util::Status Read(uint64_t n, ns_data_structure::Slice *result, char *scratch) override {
        ns_util::Status status;
        while (true) {
            ::ssize_t read_size = ::read(fd_, scratch, n);
            if (read_size < 0) {
                if (errno == EINTR) {
                    continue; // Retry
                }
                status = PosixError(filename_, errno);
            }
            *result = ns_data_structure::Slice(scratch, read_size);
            break;
        }
        return status;
    }

    ns_util::Status Skip(uint64_t n) override {
        if (::lseek(fd_, n, SEEK_CUR) == static_cast<off_t>(-1)) {
            return PosixError(filename_, errno);
        }
        return ns_util::Status::OK();
    }

private:
    int32_t const fd_;
    std::string const filename_;
};

class PosixRandomAccessFile final : public RandomAccessFile {
public:
    PosixRandomAccessFile(std::string filename, int32_t fd, Limiter *fd_limiter) :
        has_permanent_fd_(fd_limiter->Acquire()), fd_(has_permanent_fd_ ? fd : -1),
        fd_limiter_(fd_limiter), filename_(std::move(filename)) {
        if (!has_permanent_fd_) {
            assert(fd_ == -1);
            ::close(fd); // The file will be opened on every read.
        }
    }

    ~PosixRandomAccessFile() override {
        if (has_permanent_fd_) {
            assert(fd_ != -1);
            ::close(fd_);
            fd_limiter_->Release();
        }
    }

    ns_util::Status Read(uint64_t offset, uint64_t n, ns_data_structure::Slice *result, char *scratch) const override {
        int32_t fd = fd_;
        if (!has_permanent_fd_) {
            fd = ::open(filename_.c_str(), O_RDONLY | kOpenBaseFlags);
            if (fd < 0) {
                return PosixError(filename_, errno);
            }
        }

        assert(fd != -1);

        ns_util::Status status;
        ssize_t read_size = ::pread(fd, scratch, n, static_cast<off_t>(offset));
        *result = ns_data_structure::Slice(scratch, (read_size < 0) ? 0 : read_size);
        if (read_size < 0) {
            status = PosixError(filename_, errno);
        }
        if (!has_permanent_fd_) {
            assert(fd != fd_);
            ::close(fd);
        }
        return status;
    }

private:
    bool const has_permanent_fd_; // If false, the file is opened on every read.
    int32_t const fd_;
    Limiter *const fd_limiter_;
    std::string const filename_;
};

class PosixMmapReadableFile final : public RandomAccessFile {
public:
    PosixMmapReadableFile(std::string filename, char *mmap_base, uint64_t length, Limiter *mmap_limiter) :
        mmap_base_(mmap_base), length_(length),
        mmap_limiter_(mmap_limiter), filename_(std::move(filename_)) {
    }

    ~PosixMmapReadableFile() override {
        assert(::munmap(reinterpret_cast<void *>(mmap_base_), length_) == 0);
        mmap_limiter_->Release();
    }

    ns_util::Status Read(uint64_t offset, uint64_t n, ns_data_structure::Slice *result, char *scratch) const override {
        if (offset + n > length_) {
            *result = ns_data_structure::Slice();
            return PosixError(filename_, EINVAL);
        }
        *result = ns_data_structure::Slice(mmap_base_ + offset, n);
        return ns_util::Status::OK();
    }

private:
    char *const mmap_base_;
    uint64_t const length_;
    Limiter *const mmap_limiter_;
    std::string const filename_;
};

class PosixWritableFile final : public WritableFile {
public:
    PosixWritableFile(std::string filename, int32_t fd) :
        pos_(0), fd_(fd), is_manifest_(IsManifest(filename)),
        filename_(std::move(filename)), dirname_(Dirname(filename)) {
    }

    ~PosixWritableFile() override {
        if (fd_ >= 0) {
            // Ignoring any potential errors
            Close();
        }
    }

    ns_util::Status Append(ns_data_structure::Slice const &data) override {
        uint64_t write_size = data.size();
        uint8_t const *write_data = data.data();

        uint64_t copy_size = std::min(write_size, kWritableFileBufferSize - pos_);
        std::memcpy(buf_ + pos_, write_data, copy_size);
        write_data += copy_size;
        write_size -= copy_size;
        pos_ += copy_size;
        if (write_size == 0) {
            return ns_util::Status::OK();
        }
        // Can't fit in buffer, so need to do at least one write.
        ns_util::Status status = FlushBuffer();
        if (!status.ok()) {
            return status;
        }

        if (write_size < kWritableFileBufferSize) {
            std::memcpy(buf_, write_data, write_size);
            pos_ = write_size;
            return ns_util::Status::OK();
        }
        return WriteUnbuffered(write_data, write_size);
    }

    ns_util::Status Close() override {
        ns_util::Status status = FlushBuffer();
        int32_t const close_result = ::close(fd_);
        if (close_result < 0 && status.ok()) {
            status = PosixError(filename_, errno);
        }
        fd_ = -1;
        return status;
    }

    ns_util::Status Flush() override {
        return FlushBuffer();
    }

    ns_util::Status Sync() override {
        ns_util::Status status = SyncDirIfManifest();
        if (!status.ok()) {
            return status;
        }

        status = FlushBuffer();
        if (!status.ok()) {
            return status;
        }

        return SyncFd(fd_, filename_);
    }

private:
    ns_util::Status SyncDirIfManifest() {
        ns_util::Status status;
        if (!is_manifest_) {
            return status;
        }

        int32_t fd = ::open(dirname_.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd < 0) {
            status = PosixError(dirname_, errno);
        } else {
            status = SyncFd(fd, dirname_);
            ::close(fd);
        }
        return status;
    }

    ns_util::Status FlushBuffer() {
        ns_util::Status status = WriteUnbuffered(buf_, pos_);
        pos_ = 0;
        return status;
    }

    ns_util::Status WriteUnbuffered(uint8_t const *data, uint64_t size) {
        while (size > 0) {
            ssize_t write_result = ::write(fd_, data, size);
            if (write_result < 0) {
                if (errno == EINTR) {
                    continue; // Retry
                }
                return PosixError(filename_, errno);
            }
            data += write_result;
            size -= write_result;
        }
        return ns_util::Status::OK();
    }

    static ns_util::Status SyncFd(int32_t fd, std::string const &fd_path) {
#if defined(HAVE_FULLFSYNC)
        if (::fcntl(fd, F_FULLFSYNC) == 0) {
            return ns_util::Status::OK();
        }
#endif // HAVE_FULLFSYNC

#if defined(HAVE_FDATASYNC)
        bool sync_success = ::fdatasync(fd) == 0;
#else
        bool sync_success = ::fsync(fd) == 0;
#endif // HAVE_FDATASYNC
        if (sync_success) {
            return ns_util::Status::OK();
        }
        return PosixError(fd_path, errno);
    }

    static std::string Dirname(std::string const &filename) {
        std::string::size_type separator_pos = filename.rfind('/');
        if (separator_pos == std::string::npos) {
            return std::string{"."};
        }
        assert(filename.find('/', separator_pos + 1) == std::string::npos);
        return filename.substr(0, separator_pos);
    }
    static ns_data_structure::Slice Basename(std::string const &filename) {
        std::string::size_type separator_pos = filename.rfind('/');
        if (separator_pos == std::string::npos) {
            return ns_data_structure::Slice(filename);
        }
        assert(filename.find('/', separator_pos + 1) == std::string::npos);
        return ns_data_structure::Slice(filename.data() + separator_pos + 1, filename.length() - separator_pos - 1);
    }

    static bool IsManifest(std::string const &filename) {
        return Basename(filename).starts_with("MANIFEST");
    }

    uint8_t buf_[kWritableFileBufferSize];
    uint64_t pos_;
    int32_t fd_;
    const bool is_manifest_; // True if the file's name starts with MANIFEST.
    std::string const filename_;
    std::string const dirname_; // The directory of filename_.
};

int32_t LockOrUnlock(int32_t fd, bool lock) {
    errno = 0;
    struct ::flock file_lock_info;
    std::memset(&file_lock_info, 0, sizeof(file_lock_info));
    file_lock_info.l_type = (lock ? F_WRLCK : F_UNLCK);
    file_lock_info.l_whence = SEEK_SET;
    file_lock_info.l_start = 0;
    file_lock_info.l_len = 0;
    return ::fcntl(fd, F_SETLK, &file_lock_info);
}

// Instances are thread-safe because they are immutable.
class PosixFileLock : public FileLock {
public:
    PosixFileLock(int32_t fd, std::string filename) :
        fd_(fd), filename_(std::move(filename)) {
    }

    int fd() const {
        return fd_;
    }

    std::string const &filename() const {
        return filename_;
    }

private:
    int32_t const fd_;
    std::string const filename_;
};

// Tracks the files locked by PosixEnv::LockFile().
//
// We maintain a separate set instead of relying on fcntl(F_SETLK) because
// fcntl(F_SETLK) does not provide any protection against multiple uses from the
// same process.
//
// Instances are thread-safe because all member data is guarded by a mutex.
class PosixLockTable {
public:
    bool Insert(std::string const &fname) LOCKS_EXCLUDED(mutex_) {
        std::unique_lock<std::mutex> lck(mutex_);
        bool succeeded = locked_files_.insert(fname).second;
        return succeeded;
    }

    void Remove(std::string const &fname) LOCKS_EXCLUDED(mutex_) {
        std::unique_lock<std::mutex> lck(mutex_);
        locked_files_.erase(fname);
    }

private:
    std::mutex mutex_;
    std::set<std::string> locked_files_ GUARDED_BY(mutex_);
};

class PosixEnv : public Env {
public:
    PosixEnv();
    ~PosixEnv() override {
        static const char msg[] = "PosixEnv singleton destroyed. Unsupported behavior!\n";
        std::fwrite(msg, 1, sizeof(msg), stderr);
        std::abort();
    }

    ns_util::Status NewSequentialFile(std::string const &filename, SequentialFile **result) override {
        int32_t fd = ::open(filename.c_str(), O_RDONLY | kOpenBaseFlags);
        if (fd < 0) {
            *result = nullptr;
            return PosixError(filename, errno);
        }
        *result = new PosixSequentialFile(filename, fd);
        return ns_util::Status::OK();
    }

    ns_util::Status NewRandomAeccessFile(std::string const &filename, RandomAccessFile **result) override {
        *result = nullptr;
        int32_t fd = ::open(filename.c_str(), O_RDONLY | kOpenBaseFlags);
        if (fd < 0) {
            return PosixError(filename, errno);
        }

        if (!mmap_limiter_.Acquire()) {
            *result = new PosixRandomAccessFile(filename, fd, &fd_limiter_);
            return ns_util::Status::OK();
        }
        uint64_t file_size{0UL};
        ns_util::Status status = GetFileSize(filename, &file_size);
        if (status.ok()) {
            void *mmap_base = ::mmap(nullptr, file_size, PROT_READ, MAP_SHARED, fd, 0);
            if (mmap_base != MAP_FAILED) {
                *result = new PosixMmapReadableFile(filename, reinterpret_cast<char *>(mmap_base), file_size, &mmap_limiter_);
            } else {
                status = PosixError(filename, errno);
            }
        }
        ::close(fd);
        if (!status.ok()) {
            mmap_limiter_.Release();
        }
        return status;
    }

    ns_util::Status NewWritableFile(std::string const &filename, WritableFile **result) override {
        int32_t fd = ::open(filename.c_str(), O_TRUNC | O_WRONLY | O_CREAT | kOpenBaseFlags, 0644);
        if (fd < 0) {
            *result = nullptr;
            return PosixError(filename, errno);
        }
        *result = new PosixWritableFile(filename, fd);
        return ns_util::Status::OK();
    }

    ns_util::Status NewAppendableFile(std::string const &filename, WritableFile **result) override {
        int32_t fd = ::open(filename.c_str(), O_APPEND | O_WRONLY | O_CREAT | kOpenBaseFlags, 0644);
        if (fd < 0) {
            *result = nullptr;
            return PosixError(filename, errno);
        }
        *result = new PosixWritableFile(filename, fd);
        return ns_util::Status::OK();
    }

    bool FileExists(std::string const &filename) override {
        return ::access(filename.c_str(), F_OK) == 0;
    }

    ns_util::Status GetChildren(std::string const &directory_path, std::vector<std::string> *result) override {
        result->clear();
        ::DIR *dir = ::opendir(directory_path.c_str());
        if (dir == nullptr) {
            return PosixError(directory_path, errno);
        }
        struct ::dirent *entry;
        while ((entry = ::readdir(dir)) != nullptr) {
            result->emplace_back(entry->d_name);
        }
        ::closedir(dir);
        return ns_util::Status::OK();
    }

    ns_util::Status RemoveFile(std::string const &filename) override {
        if (::unlink(filename.c_str()) != 0) {
            return PosixError(filename, errno);
        }
        return ns_util::Status::OK();
    }

    ns_util::Status CreateDir(std::string const &dirname) override {
        if (::mkdir(dirname.c_str(), 0755) != 0) {
            return PosixError(dirname, errno);
        }
        return ns_util::Status::OK();
    }

    ns_util::Status RemoveDir(std::string const &dirname) override {
        if (::rmdir(dirname.c_str()) != 0) {
            return PosixError(dirname, errno);
        }
        return ns_util::Status::OK();
    }

    ns_util::Status GetFileSize(std::string const &filename, uint64_t *size) override {
        struct ::stat file_stat;
        if (::stat(filename.c_str(), &file_stat) != 0) {
            *size = 0;
            return PosixError(filename, errno);
        }
        *size = file_stat.st_size;
        return ns_util::Status::OK();
    }

    ns_util::Status RenameFile(std::string const &from, std::string const &to) override {
        if (std::rename(from.c_str(), to.c_str()) != 0) {
            return PosixError(from, errno);
        }
        return ns_util::Status::OK();
    }

    ns_util::Status LockFile(std::string const &filename, FileLock **lock) override {
        *lock = nullptr;
        int32_t fd = ::open(filename.c_str(), O_RDWR | O_CREAT | kOpenBaseFlags, 0644);
        if (fd < 0) {
            return PosixError(filename, errno);
        }
        if (!locks_.Insert(filename)) {
            ::close(fd);
            return ns_util::Status::IOError("lock " + filename, "already held by process");
        }
        if (LockOrUnlock(fd, true) == -1) {
            int32_t lock_errno = errno;
            ::close(fd);
            locks_.Remove(filename);
            return PosixError("lock " + filename, lock_errno);
        }
        *lock = new PosixFileLock(fd, filename);
        return ns_util::Status::OK();
    }

    ns_util::Status UnlockFile(FileLock *lock) override {
        PosixFileLock *posix_file_lock = static_cast<PosixFileLock *>(lock);
        if (LockOrUnlock(posix_file_lock->fd(), false) == -1) {
            return PosixError("unlock " + posix_file_lock->filename(), errno);
        }
        locks_.Remove(posix_file_lock->filename());
        ::close(posix_file_lock->fd());
        delete posix_file_lock;
        return ns_util::Status::OK();
    }

    void Schedule(TaskFunc const &function, void *arg) override;

    void StartThread(TaskFunc const &function, void *arg) override {
        std::thread new_thread(function, arg);
        new_thread.detach();
    }

    ns_util::Status GetTestDirectory(std::string *result) override {
        char const *env = std::getenv("TEST_TMPDIR");
        if (env && env[0] != '\0') {
            *result = env;
        } else {
            char buf[100];
            std::snprintf(buf, sizeof(buf), "/tmp/leveldbtest-%d", static_cast<int32_t>(::geteuid()));
            *result = buf;
        }
        // The CreateDir status is ignored because the directory may already exist.
        CreateDir(*result);
        return ns_util::Status::OK();
    }

    ns_util::Status NewLogger(std::string const &filename, Logger **result) override {
        int32_t fd = ::open(filename.c_str(), O_APPEND | O_WRONLY | O_CREAT | kOpenBaseFlags, 0644);
        if (fd < 0) {
            *result = nullptr;
            return PosixError(filename, errno);
        }
        std::FILE *fp = ::fdopen(fd, "w");
        if (fp == nullptr) {
            ::close(fd);
            *result = nullptr;
            return PosixError(filename, errno);
        }
        *result = new ns_log::PosixLogger(fp);
        return ns_util::Status::OK();
    }

    uint64_t NowMicros() override {
        static constexpr uint64_t kUsecondsPerSecond = 1000000;
        struct ::timeval tv;
        ::gettimeofday(&tv, nullptr);
        return static_cast<uint64_t>(tv.tv_sec) * kUsecondsPerSecond + tv.tv_usec;
    }

    void SleepForMicroseconds(int32_t micros) override {
        std::this_thread::sleep_for(std::chrono::microseconds(micros));
    }

private:
    void BackgroundThreadMain();

    static void BackgroundThreadEntryPoint(PosixEnv *env) {
        env->BackgroundThreadMain();
    }
    // Stores the work item data in a Schedule() call.
    //
    // Instances are constructed on the thread calling Schedule() and used on the
    // background thread.
    //
    // This structure is thread-safe because it is immutable.
    struct BackgroundWorkItem {
        explicit BackgroundWorkItem(TaskFunc const &func, void *ag) :
            function(func), arg(ag) {
        }
        TaskFunc function;
        void *const arg;
    };

    std::mutex background_work_mutex_;
    std::condition_variable background_work_cv_ GUARDED_BY(background_work_mutex_);
    bool started_background_thread_ GUARDED_BY(background_work_mutex_);

    std::queue<BackgroundWorkItem> background_work_queue_ GUARDED_BY(background_work_mutex_);

    PosixLockTable locks_; // Thread-safe
    Limiter mmap_limiter_; // Thread-safe
    Limiter fd_limiter_;   // Thread-safe
};

int32_t MaxMmaps() {
    return g_mmap_limit;
}
// Return the maximum number of read-only files to keep open.
int32_t MaxOpenFiles() {
    if (g_open_read_only_file_limit >= 0) {
        return g_open_read_only_file_limit;
    }
#ifdef __Fuchsia__
    g_open_read_only_file_limit = 50;
#else
    struct ::rlimit rlim;
    if (::getrlimit(RLIMIT_NOFILE, &rlim)) {
        g_open_read_only_file_limit = 50;
    } else if (rlim.rlim_cur == RLIM_INFINITY) {
        g_open_read_only_file_limit = std::numeric_limits<int32_t>::max();
    } else {
        // Allow use of 20% of available file descriptors for read-only files.
        g_open_read_only_file_limit = rlim.rlim_cur / 5;
    }
#endif // __Fuchsia__
    return g_open_read_only_file_limit;
}

} // anonymous namespace

PosixEnv::PosixEnv() :
    started_background_thread_(false),
    mmap_limiter_(MaxMmaps()),
    fd_limiter_(MaxOpenFiles()) {
}

void PosixEnv::Schedule(TaskFunc const &function, void *arg) {
    std::unique_lock<std::mutex> lck(background_work_mutex_);
    // Start the background thread, if we haven't done so already.
    if (!started_background_thread_) {
        started_background_thread_ = true;
        std::thread background_thread(PosixEnv::BackgroundThreadEntryPoint, this);
        background_thread.detach();
    }
    // If the queue is empty, the background thread may be waiting for work.so notify one thread to work
    if (background_work_queue_.empty()) {
        background_work_cv_.notify_one();
    }
    background_work_queue_.emplace(function, arg);
}

void PosixEnv::BackgroundThreadMain() {
    while (true) {
        std::unique_lock<std::mutex> lck(background_work_mutex_);
        // Wait until there is work to be done.
        while (background_work_queue_.empty()) {
            background_work_cv_.wait(lck);
        }
        assert(!background_work_queue_.empty());
        auto background_work_function = background_work_queue_.front().function;
        void *background_work_arg = background_work_queue_.front().arg;
        background_work_queue_.pop();
        background_work_function(background_work_arg);
    }
}

namespace {
// Wraps an Env instance whose destructor is never created.
//
// Intended usage:
//   using PlatformSingletonEnv = SingletonEnv<PlatformEnv>;
//   void ConfigurePosixEnv(int param) {
//     PlatformSingletonEnv::AssertEnvNotInitialized();
//     // set global configuration flags.
//   }
//   Env* Env::Default() {
//     static PlatformSingletonEnv default_env;
//     return default_env.env();
//   }
template <typename EnvType>
class SingletonEnv {
public:
    SingletonEnv() {
#if !defined(NDEBUG)
        env_initialized_.store(true, std::memory_order_relaxed);
#endif // !defined(NDEBUG)
        static_assert(sizeof(env_storage_) >= sizeof(EnvType), "env_storage_ will not fit the Env");
        static_assert(alignof(decltype(env_storage_)) >= alignof(EnvType), "env_storage_ does not meet the Env's alignment needs");
        new (&env_storage_) EnvType();
    }

    ~SingletonEnv() = default;

    SingletonEnv(SingletonEnv const &) = delete;
    SingletonEnv &operator=(SingletonEnv const &) = delete;

    Env *env() {
        return reinterpret_cast<Env *>(&env_storage_);
    }

    static void AssertEnvNotInitialized() {
#if !defined(NDEBUG)
        assert(!env_initialized_.load(std::memory_order_relaxed));
#endif // !defined(NDEBUG)
    }

private:
    typename std::aligned_storage<sizeof(EnvType), alignof(EnvType)>::type env_storage_;
#if !defined(NDEBUG)
    static std::atomic<bool> env_initialized_;
#endif // !defined(NDEBUG)
};

#if !defined(NDEBUG)
template <typename EnvType>
std::atomic<bool> SingletonEnv<EnvType>::env_initialized_;
#endif // !defined(NDEBUG)

using PosixDefaultEnv = SingletonEnv<PosixEnv>;

} // anonymous namespace

void EnvPosixTestHelper::SetReadOnlyFDLimit(int32_t limit) {
    PosixDefaultEnv::AssertEnvNotInitialized();
    g_open_read_only_file_limit = limit;
}

void EnvPosixTestHelper::SetReadOnlyMMapLimit(int32_t limit) {
    PosixDefaultEnv::AssertEnvNotInitialized();
    g_mmap_limit = limit;
}

Env *Env::Default() {
    static PosixDefaultEnv env_container;
    return env_container.env();
}
} // ns_env