#include "skiplist.h"
#include <cstdint>

using namespace ns_data_structure::ns_skip_list;
using namespace ns_memory::ns_arena;

using Key = uint64_t;
struct Comparator {
    int32_t operator()( Key const& a, Key const& b)const {
        if(a < b) {
            return -1;
        } else if(a > b) {
            return 1;
        }
        return 0;
    }
};

void check1() {
    {
        Arena arena;
        Comparator cmp;
        SkipList<Key, Comparator> list(cmp, &arena);
        PRINT_INFO("%d", list.Contains(10));
    }
}

int main() {
    check1();
    return 0;
}