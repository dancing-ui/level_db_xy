#include "coding.h"

namespace ns_util {

void PutFixed64(std::string* dst, uint64_t value) {
    uint8_t buf[sizeof(value)];
    EncodeFixed64(buf, value);
    dst->append(reinterpret_cast<char*>(buf), sizeof(buf));
}

} // ns_util