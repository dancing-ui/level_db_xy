#ifndef _LEVEL_DB_XY_STATUS_H_
#define _LEVEL_DB_XY_STATUS_H_

#include "slice.h"

namespace ns_util {

class Status {
public:
    Status() noexcept :
        state_(nullptr) {
    }
    ~Status() {
        delete[] state_;
    }

    Status(Status const &rhs) {
        state_ = (rhs.state_ == nullptr) ? nullptr : CopyState(rhs.state_);
    }
    Status &operator=(Status const &rhs) {
        if(state_ != rhs.state_) {
            delete[] state_;
            state_ = (rhs.state_ == nullptr) ? nullptr : CopyState(rhs.state_);
        }
        return *this;
    }

    Status(Status &&rhs) noexcept :
        state_(rhs.state_) {
        rhs.state_ = nullptr;
    }
    Status &operator=(Status &&rhs) noexcept {
        std::swap(state_, rhs.state_);
        return *this;
    }

    static Status OK() {
        return Status();
    }
    static Status NotFound(ns_data_structure::Slice const &msg, ns_data_structure::Slice const &msg2 = ns_data_structure::Slice()) {
        return Status(Code::kNotFound, msg, msg2);
    }
    static Status Corruption(ns_data_structure::Slice const &msg, ns_data_structure::Slice const &msg2 = ns_data_structure::Slice()) {
        return Status(Code::kCorruption, msg, msg2);
    }
    static Status NotSupported(ns_data_structure::Slice const &msg, ns_data_structure::Slice const &msg2 = ns_data_structure::Slice()) {
        return Status(Code::kNotSupported, msg, msg2);
    }
    static Status InvalidArgument(ns_data_structure::Slice const &msg, ns_data_structure::Slice const &msg2 = ns_data_structure::Slice()) {
        return Status(Code::kInvalidArgument, msg, msg2);
    }
    static Status IOError(ns_data_structure::Slice const &msg, ns_data_structure::Slice const &msg2 = ns_data_structure::Slice()) {
        return Status(Code::kIOError, msg, msg2);
    }

    bool ok() const {
        return state_ == nullptr;
    }
    bool IsNotFound() const {
        return code() == Code::kNotFound;
    }
    bool IsCorruption() const {
        return code() == Code::kCorruption;
    }
    bool IsNotSupported() const {
        return code() == Code::kNotSupported;
    }
    bool IsInvalidArgument() const {
        return code() == Code::kInvalidArgument;
    }
    bool IsIOError() const {
        return code() == Code::kIOError;
    }

    std::string ToString() const;

private:
    enum Code {
        kOk = 0,
        kNotFound = 1,
        kCorruption = 2,
        kNotSupported = 3,
        kInvalidArgument = 4,
        kIOError = 5
    };

    Code code() const {
        return (state_ == nullptr) ? Code::kOk : static_cast<Code>(state_[4]);
    }

    Status(Code code, ns_data_structure::Slice const &msg, ns_data_structure::Slice const& msg2);
    static char const *CopyState(char const *state);
    // OK status has a null state_.  Otherwise, state_ is a new[] array
    // of the following form:
    //    state_[0..3] == length of message
    //    state_[4]    == code
    //    state_[5..]  == message
    char const *state_;
};

} // ns_util

#endif