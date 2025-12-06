#include <iostream>
#include <cstring>

extern "C" double nova_random();

int main() {
    std::cout << "Testing nova_random() directly:\n";
    for (int i = 0; i < 10; i++) {
        double r = nova_random();
        std::cout << "  Random " << i << ": " << r << std::endl;

        // Also show as bitcasted i64
        int64_t bits;
        std::memcpy(&bits, &r, sizeof(double));
        std::cout << "    (as i64: " << bits << ")" << std::endl;
    }
    return 0;
}
