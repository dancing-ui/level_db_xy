#include "slice.h"

using namespace ns_data_structure::ns_slice;

void check1() {
    {
        Slice slice;
        std::string str{"hello, world"};
        slice.SetData(str, str.size());
        printf("len=%d, data=%s\n", slice.GetLen(), slice.GetData());
    }
    {
        Slice slice2{"hello", 5};
        printf("len=%d, data=%s\n", slice2.GetLen(), slice2.GetData());
    }
    {
        Slice slice1{"world", 5};
        Slice slice2{};
        slice2 = slice1;
        printf("len=%d, data=%s\n", slice2.GetLen(), slice2.GetData());
    }
}

int main() {
    check1();
    return 0;
}