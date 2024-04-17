#ifndef _LEVEL_DB_XY_ENV_H_
#define _LEVEL_DB_XY_ENV_H_

#include "status.h"
#include "slice.h"

#include <cstdarg>
#include <vector>
#include <functional>

namespace ns_env {

using TaskFunc = std::function<void(void *)>;

class WritableFile {
public:
    WritableFile() = default;

    WritableFile(WritableFile const &) = delete;
    WritableFile &operator=(WritableFile const &) = delete;

    virtual ~WritableFile() = default;

    virtual ns_util::Status Append(ns_data_structure::Slice const &data) = 0;
    virtual ns_util::Status Close() = 0;
    virtual ns_util::Status Flush() = 0;
    virtual ns_util::Status Sync() = 0;
};

class SequentialFile {
public:
    SequentialFile() = default;

    SequentialFile(SequentialFile const &) = delete;
    SequentialFile &operator=(SequentialFile const &) = delete;

    virtual ~SequentialFile() = default;
    virtual ns_util::Status Read(uint64_t n, ns_data_structure::Slice *result, char *scratch) = 0;
    virtual ns_util::Status Skip(uint64_t n) = 0;
};

class RandomAccessFile {
public:
    RandomAccessFile() = default;

    RandomAccessFile(RandomAccessFile const &) = delete;
    RandomAccessFile &operator=(RandomAccessFile const &) = delete;

    virtual ~RandomAccessFile() = default;

    // Read up to "n" bytes from the file starting at "offset".
    // "scratch[0..n-1]" may be written by this routine.  Sets "*result"
    // to the data that was read (including if fewer than "n" bytes were
    // successfully read).  May set "*result" to point at data in
    // "scratch[0..n-1]", so "scratch[0..n-1]" must be live when
    // "*result" is used.  If an error was encountered, returns a non-OK
    // status.
    //
    // Safe for concurrent use by multiple threads.
    virtual ns_util::Status Read(uint64_t offset, uint64_t n, ns_data_structure::Slice *result, char *scratch) const = 0;
};

class FileLock {
public:
    FileLock() = default;
    FileLock(FileLock const &) = delete;
    FileLock &operator=(FileLock const &) = delete;
    virtual ~FileLock() = default;
};

class Logger {
public:
    Logger() = default;
    Logger(Logger const &) = delete;
    Logger &operator=(Logger const &) = delete;
    virtual ~Logger() = default;
    // Write an entry to the log file with the specified format.
    virtual void Logv(char const *format, std::va_list ap) = 0;
};

class Env {
public:
    Env() = default;
    Env(Env const &) = delete;
    Env &operator=(Env const &) = delete;

    virtual ~Env() = default;

    static Env *Default();

    virtual ns_util::Status NewSequentialFile(std::string const &fname, SequentialFile **result) = 0;

    virtual ns_util::Status NewRandomAeccessFile(std::string const &fname, RandomAccessFile **result) = 0;

    virtual ns_util::Status NewWritableFile(std::string const &fname, WritableFile **result) = 0;

    virtual ns_util::Status NewAppendableFile(std::string const &fname, WritableFile **result);

    virtual bool FileExists(std::string const &fname) = 0;

    virtual ns_util::Status GetChildren(std::string const &dir, std::vector<std::string> *result) = 0;

    virtual ns_util::Status RemoveFile(std::string const &fname);

    virtual ns_util::Status DeleteFile(std::string const &fname);

    virtual ns_util::Status CreateDir(std::string const &dirname) = 0;

    virtual ns_util::Status RemoveDir(std::string const &dirname);

    virtual ns_util::Status DeleteDir(std::string const &dirname);

    virtual ns_util::Status GetFileSize(std::string const &fname, uint64_t *file_size) = 0;

    virtual ns_util::Status RenameFile(std::string const &src, std::string const &target) = 0;

    virtual ns_util::Status LockFile(std::string const &fname, FileLock **lock) = 0;

    virtual ns_util::Status UnlockFile(FileLock *lock) = 0;

    virtual void Schedule(TaskFunc const &function, void *arg) = 0;

    virtual void StartThread(TaskFunc const &function, void *arg) = 0;

    virtual ns_util::Status GetTestDirectory(std::string *path) = 0;

    virtual ns_util::Status NewLogger(std::string const &fname, Logger **result) = 0;

    virtual uint64_t NowMicros() = 0;

    virtual void SleepForMicroseconds(int32_t micros) = 0;
};

class EnvWrapper : public Env {
public:
    explicit EnvWrapper(Env *t) :
        target_(t) {
    }

    virtual ~EnvWrapper() = default;

    Env *target() const {
        return target_;
    }

    ns_util::Status NewSequentialFile(std::string const &fname, SequentialFile **result) override {
        return target_->NewSequentialFile(fname, result);
    }

    ns_util::Status NewRandomAeccessFile(std::string const &fname, RandomAccessFile **result) override {
        return target_->NewRandomAeccessFile(fname, result);
    }

    ns_util::Status NewWritableFile(std::string const &fname, WritableFile **result) override {
        return target_->NewWritableFile(fname, result);
    }

    ns_util::Status NewAppendableFile(std::string const &fname, WritableFile **result) override {
        return target_->NewAppendableFile(fname, result);
    }

    bool FileExists(std::string const &fname) override {
        return target_->FileExists(fname);
    }

    ns_util::Status GetChildren(std::string const &dir, std::vector<std::string> *result) override {
        return target_->GetChildren(dir, result);
    }

    ns_util::Status RemoveFile(std::string const &fname) override {
        return target_->RemoveFile(fname);
    }

    ns_util::Status CreateDir(std::string const &dirname) override {
        return target_->CreateDir(dirname);
    }

    ns_util::Status RemoveDir(std::string const &dirname) override {
        return target_->RemoveDir(dirname);
    }

    ns_util::Status GetFileSize(std::string const &fname, uint64_t *file_size) override {
        return target_->GetFileSize(fname, file_size);
    }

    ns_util::Status RenameFile(std::string const &src, std::string const &target) override {
        return target_->RenameFile(src, target);
    }

    ns_util::Status LockFile(std::string const &fname, FileLock **lock) override {
        return target_->LockFile(fname, lock);
    }

    ns_util::Status UnlockFile(FileLock *lock) override {
        return target_->UnlockFile(lock);
    }

    void StartThread(TaskFunc const &function, void *arg) override {
        return target_->StartThread(function, arg);
    }

    ns_util::Status GetTestDirectory(std::string *path) override {
        return target_->GetTestDirectory(path);
    }

    ns_util::Status NewLogger(std::string const &fname, Logger **result) override {
        return target_->NewLogger(fname, result);
    }

    uint64_t NowMicros() override {
        return target_->NowMicros();
    }

    void SleepForMicroseconds(int32_t micros) override {
        target_->SleepForMicroseconds(micros);
    }

private:
    Env *target_;
};

void Log(Logger *info_log, char const *format, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((__format__(__printf__, 2, 3)))
#endif
    ;

} // ns_env

#endif