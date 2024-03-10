#ifndef _SLICE_H_
#define _SLICE_H_

#include <cstdint>
#include <string>

namespace ns_data_structure {
namespace ns_slice {

class Slice {
public:
    Slice();
    ~Slice() = default;
    Slice(uint8_t const *data, int32_t len);
    Slice(std::string const &data, int32_t len);
    Slice(Slice const &slice);
    Slice(Slice &&slice);
    Slice &operator=(Slice const &slice);
    Slice &operator=(Slice &&slice);

    
    int32_t GetLen() const;
    uint8_t const *GetData() const;
    void SetData(uint8_t const *data, int32_t len);
    void SetData(std::string const &data, int32_t len);
    std::string ToStdString();
    bool Empty();
    int32_t Compare();

private:
    int32_t len_{0};
    uint8_t const *data_{nullptr};
};

}
} // namespace ns_data_structure::ns_slice

#endif