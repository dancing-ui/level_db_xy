#include "slice.h"

using namespace ns_data_structure;

void check1() {
    Slice a("helloworld");
    Slice b(a);
    a.remove_prefix(5);
    printf("a=%s b=%s\n", a.ToString().c_str(), b.ToString().c_str());
}

int main() {
    check1();
    return 0;
}