#include "coding.h"

namespace ns_util {

void PutVarint32(std::string* dst, uint32_t value) {
    uint8_t buf[5];
    uint8_t* ptr = EncodeVarint32(buf, value);
    dst->append(reinterpret_cast<char*>(buf), ptr - buf);
}

void PutFixed32(std::string* dst, uint32_t value) {
    uint8_t buf[sizeof(value)];
    EncodeFixed32(buf, value);
    dst->append(reinterpret_cast<char*>(buf), sizeof(buf));
}

void PutFixed64(std::string *dst, uint64_t value) {
    uint8_t buf[sizeof(value)];
    EncodeFixed64(buf, value);
    dst->append(reinterpret_cast<char *>(buf), sizeof(buf));
}

void PutLengthPrefixedSlice(std::string* dst, ns_data_structure::Slice const& value) {
    PutVarint32(dst, value.size());
    dst->append(reinterpret_cast<char const*>(value.data()), value.size());
}

bool GetVarint32(ns_data_structure::Slice* input, uint32_t* value) {
    uint8_t const* p = input->data();
    uint8_t const* limit = p + input->size();
    uint8_t const* q = GetVarint32Ptr(p, limit, value);
    if(q == nullptr) {
        return false;
    }
    *input = ns_data_structure::Slice(q, limit - q);
    return true;
}

bool GetVarint64(ns_data_structure::Slice* input, uint64_t* value) {
    uint8_t const* p = input->data();
    uint8_t const* limit = p + input->size();
    uint8_t const* q = GetVarint64Ptr(p, limit, value);
    if(q == nullptr) {
        return false;
    }
    *input = ns_data_structure::Slice(q, limit - q);
    return true;
}

bool GetLengthPrefixedSlice(ns_data_structure::Slice * input, ns_data_structure::Slice* result) {
    uint32_t len;
    if(GetVarint32(input, &len) && input->size() >= len) {
        *result = ns_data_structure::Slice(input->data(), len);
        input->remove_prefix(len);
        return true;
    }
    return false;
}

uint8_t *EncodeVarint32(uint8_t *dst, uint32_t value) {
    static constexpr uint8_t B = 1 << 7;
    if (value < (1U << 7)) {
        *(dst++) = value;
    } else if (value < (1U << 14)) {
        *(dst++) = value | B;
        *(dst++) = value >> 7;
    } else if (value < (1U << 21)) {
        *(dst++) = value | B;
        *(dst++) = (value >> 7) | B;
        *(dst++) = value >> 14;
    } else if (value < (1ULL << 28)) {
        *(dst++) = value | B;
        *(dst++) = (value >> 7) | B;
        *(dst++) = (value >> 14) | B;
        *(dst++) = value >> 21;
    } else {
        *(dst++) = value | B;
        *(dst++) = (value >> 7) | B;
        *(dst++) = (value >> 14) | B;
        *(dst++) = (value >> 21) | B;
        *(dst++) = value >> 28;
    }
    return dst;
}

uint8_t const *GetVarint32Ptr(uint8_t const *p, uint8_t const *limit, uint32_t *value) {
    uint32_t result{0U};
    static constexpr uint8_t B = 1 << 7;
    for (uint32_t shift = 0U; shift <= 28 && p < limit; shift += 7) {
        uint32_t byte = *p;
        p++;
        if (byte & B) {
            result |= ((byte & 127) << shift);
        } else {
            result |= (byte << shift);
            *value = result;
            return p;
        }
    }
    return nullptr;
}

uint8_t const *GetVarint64Ptr(uint8_t const *p, uint8_t const *limit, uint64_t *value) {
    uint64_t result{0UL};
    static constexpr uint8_t B = 1 << 7;
    for (uint32_t shift = 0U; shift <= 63 && p < limit; shift += 7) {
        uint64_t byte = *p;
        p++;
        if (byte & B) {
            result |= ((byte & 127) << shift);
        } else {
            result |= (byte << shift);
            *value = result;
            return p;
        }
    }
    return nullptr;
}

int32_t VarintLength(uint64_t value) {
    static constexpr uint8_t B = 1 << 7;
    int32_t len = 1;
    while(value >= B) {
        value >>= 7;
        len++;
    }
    return len;
}

} // ns_util