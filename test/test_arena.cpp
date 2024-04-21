#include "arena.h"
using namespace ns_memory;

void check1() {
    {
        Arena arena;
        arena.Allocate(10);
    }
}

int main() {
    check1();
    return 0;
}