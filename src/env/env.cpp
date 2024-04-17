#include "env.h"

namespace ns_env {

ns_util::Status Env::NewAppendableFile(std::string const &fname, WritableFile **result) {
    return ns_util::Status::NotSupported("NewAppendableFile", fname);
}

ns_util::Status Env::RemoveFile(std::string const &fname) {
    return DeleteFile(fname);
}

ns_util::Status Env::DeleteFile(std::string const &fname) {
    return RemoveFile(fname);
}

ns_util::Status Env::RemoveDir(std::string const &dirname) {
    return DeleteDir(dirname);
}

ns_util::Status Env::DeleteDir(std::string const &dirname) {
    return RemoveDir(dirname);
}

void Log(Logger *info_log, char const *format, ...) {
    if (info_log != nullptr) {
        std::va_list ap;
        va_start(ap, format);
        info_log->Logv(format, ap);
        va_end(ap);
    }
}

static ns_util::Status DoWriteStringToFile(Env *env, ns_data_structure::Slice const &data,
                                           std::string const &fname, bool should_sync) {
    WritableFile *file;
    ns_util::Status s = env->NewWritableFile(fname, &file);
    if (!s.ok()) {
        return s;
    }
    s = file->Append(data);
    if (s.ok() && should_sync) {
        s = file->Sync();
    }
    if (s.ok()) {
        s = file->Close();
    }
    delete file;
    if (!s.ok()) {
        env->RemoveFile(fname);
    }
    return s;
}

ns_util::Status WriteStringToFile(Env *env, ns_data_structure::Slice const &data,
                                  std::string const &fname, bool should_sync) {
    return DoWriteStringToFile(env, data, fname, false);
}

ns_util::Status WriteStringToFileSync(Env *env, ns_data_structure::Slice const &data,
                                  std::string const &fname, bool should_sync) {
    return DoWriteStringToFile(env, data, fname, true);
}

ns_util::Status ReadFileToString(Env* env, std::string const& fname, std::string* data) {
    data->clear();
    SequentialFile* file;
    ns_util::Status s = env->NewSequentialFile(fname, &file);
    if(!s.ok()) {
        return s;
    }
    static constexpr int32_t kBufferSize = 8192;
    char* space = new char[kBufferSize];
    while(true) {
        ns_data_structure::Slice fragment;
        s = file->Read(kBufferSize, &fragment, space);
        if(!s.ok()) {
            break;
        }
        data->append(reinterpret_cast<char const*>(fragment.data()), fragment.size());
        if(fragment.empty()) {
            break;
        }
    }
    delete[] space;
    delete file;
    return s;
}

} // ns_env