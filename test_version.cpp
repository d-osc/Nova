#include <iostream>
#include "include/nova/Version.h"

int main() {
    std::cout << "Version from header: " << NOVA_VERSION << std::endl;
    std::cout << "Version string: " << NOVA_VERSION_STRING << std::endl;
    std::cout << "Version components: " << NOVA_VERSION_MAJOR << "." << NOVA_VERSION_MINOR << "." << NOVA_VERSION_PATCH << std::endl;
    return 0;
}