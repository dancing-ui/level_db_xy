#include "slice.h"

namespace ns_data_structure {
namespace ns_slice {

Slice::Slice() {
    data_ = nullptr;
    len_ = 0;
}
Slice::Slice(uint8_t const *data, int32_t len) {
    data_ = data;
    len_ = len;
}
Slice::Slice(std::string const &data, int32_t len) {
    data_ = reinterpret_cast<uint8_t const*>(data.data());
    len_ = len;
}
Slice::Slice(Slice const &slice) {
    data_ = slice.GetData();
    len_ = slice.GetLen();
}
Slice::Slice(Slice &&slice) {
    data_ = slice.GetData();
    len_ = slice.GetLen();
}
Slice &Slice::operator=(Slice const &slice) {
    data_ = slice.GetData();
    len_ = slice.GetLen();
    return *this;
}
Slice &Slice::operator=(Slice &&slice) {
    data_ = slice.GetData();
    len_ = slice.GetLen();
    return *this;
}
int32_t Slice::GetLen() const {
    return len_;
}
uint8_t const *Slice::GetData() const {
    return data_;
}
void Slice::SetData(uint8_t const *data, int32_t len) {
    data_ = data;
    len_ = len;
}
void Slice::SetData(std::string const &data, int32_t len) {
    data_ = reinterpret_cast<uint8_t const*>(data.data());
    len_ = len;
}
std::string Slice::ToStdString() {
    std::string ret{};
    for (int32_t i = 0; i < len_; i++) {
        ret.push_back(data_[i]);
    }
    return ret;
}

} // ns_slice
} // ns_data_structure
