/**
 * Nova Built-in Modules Registry
 *
 * Central registry for all nova: prefixed modules.
 */

#include "nova/runtime/BuiltinModules.h"
#include <algorithm>

namespace nova {
namespace runtime {

// List of available built-in modules
static const std::vector<std::string> BUILTIN_MODULES = {
    "nova:fs",
    "nova:test",
    "nova:path",
    "nova:os"
};

// Check if module is a built-in nova module
bool isBuiltinModule(const std::string& modulePath) {
    // Check for nova: prefix
    if (modulePath.length() < 5 || modulePath.substr(0, 5) != "nova:") {
        return false;
    }

    // Check if it's in our list
    return std::find(BUILTIN_MODULES.begin(), BUILTIN_MODULES.end(), modulePath) != BUILTIN_MODULES.end();
}

// Get list of available built-in modules
std::vector<std::string> getBuiltinModules() {
    return BUILTIN_MODULES;
}

} // namespace runtime
} // namespace nova
