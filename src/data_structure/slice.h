#ifndef _LEVEL_DB_XY_SLICE_H_
#define _LEVEL_DB_XY_SLICE_H_

#include <cstdint>
#include <string>
#include <cstring>
#include <cassert>

namespace ns_data_structure {

class Slice {
public:
    Slice() : data_(nullptr), size_(0) {}

    Slice(char const* data) : data_(reinterpret_cast<uint8_t const*>(data)), size_(strlen(data)) {}
    Slice(char const* data, uint64_t size) : data_(reinterpret_cast<uint8_t const*>(data)), size_(size) {}

    Slice(uint8_t const * data) : data_(data), size_(strlen(reinterpret_cast<char const*>(data))) {}
    Slice(uint8_t const *data, uint64_t size) : data_(data), size_(size) {}

    Slice(std::string const & data) : data_(reinterpret_cast<uint8_t const*>(data.data())), size_(data.size()) {};

    Slice(Slice const &slice) = default;
    Slice &operator=(Slice const &slice) = default;

    uint64_t size() const { return size_; };
    uint8_t const *data() const { return data_; };
    bool empty() const { return size_ == 0; }

    uint8_t operator[](uint64_t n) const {
        assert(n < size());
        return data_[n];
    }
    bool operator==(Slice const& b) const {
        return ((size() == b.size()) && (memcmp(data(), b.data(), size()) == 0));
    }
    bool operator!=(Slice const& b) const {
        return !(*this == b);
    }

    void clear() {
        data_ = nullptr;
        size_ = 0;
    }
    void remove_prefix(uint64_t n) {
        assert(n <= size());
        data_ += n;
        size_ -= n;
    }
    std::string ToString() const {
        return std::string{reinterpret_cast<char const*>(data_), size_};
    }
    int32_t compare(Slice const& b) const {
        uint64_t const min_len = (size() < b.size()) ? size() : b.size();
        int32_t ret = memcmp(data(), b.data(), min_len);
        if(ret == 0) {
            if(size() < b.size()) {
                ret = -1;
            } else {
                ret = 1;
            }
        }
        return ret;
    }
    bool start_with(Slice const& x) const {
        return ((size() >= x.size()) && (memcmp(data(), x.data(), x.size()) == 0));
    }

private:
    uint8_t const *data_;
    uint64_t size_;
};


} // ns_data_structure

#endif