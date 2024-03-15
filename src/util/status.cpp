#include "status.h"

namespace ns_util {

Status::Status(Code code, ns_data_structure::Slice const &msg, ns_data_structure::Slice const &msg2) {
    assert(code != Code::kOk);
    uint32_t const len1 = static_cast<uint32_t>(msg.size());
    uint32_t const len2 = static_cast<uint32_t>(msg2.size());
    uint32_t const size = len1 + (len2 ? (2 + len2) : 0);
    char *result = new char[size + 5];
    std::memcpy(result, &size, sizeof(size));
    result[4] = static_cast<char>(code);
    std::memcpy(result + 5, msg.data(), len1);
    if (len2) {
        result[5 + len1] = ':';
        result[6 + len1] = ' ';
        std::memcpy(result + 7 + len1, msg2.data(), len2);
    }
    state_ = result;
}
char const *Status::CopyState(char const *state) {
    uint32_t size;
    std::memcpy(&size, state, sizeof(size));
    char *result = new char[size + 5];
    std::memcpy(result, state, size + 5);
    return result;
}
std::string Status::ToString() const {
    if(state_ == nullptr) {
        return "OK";
    } else {
        char tmp[30];
        char const* type;
        switch (code())
        {
        case Code::kOk:
            type = "OK";
            break;
        case Code::kNotFound:
            type = "NotFound: ";
            break;
        case Code::kCorruption:
            type = "Corruption: ";
            break;
        case Code::kNotSupported:
            type = "Not implemented: ";
            break;
        case Code::kInvalidArgument:
            type = "Invalid argument: ";
            break;
        case Code::kIOError:
            type = "IO error: ";
            break;
        default:
            std::snprintf(tmp, sizeof(tmp), "Unknown code(%d): ", static_cast<int32_t>(code()));
            type = tmp;
            break;
        }
        std::string result(type);
        uint32_t length;
        std::memcpy(&length, state_, sizeof(length));
        result.append(state_ + 5, length);
        return result;
    }
    return "";
}

} // ns_util
