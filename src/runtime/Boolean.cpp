// Boolean runtime functions for Nova compiler
// Implements ES1 Boolean prototype methods

#include <cstdint>
#include <cstring>
#include <cstdlib>

extern "C" {

// ============================================
// Boolean.prototype.toString()
// Returns "true" or "false" based on boolean value
// ============================================
void* nova_boolean_toString(int64_t value) {
    // Allocate string for result
    // "false" is 5 chars, "true" is 4 chars
    if (value) {
        // true
        char* result = static_cast<char*>(malloc(5));
        if (result) {
            memcpy(result, "true", 5);  // includes null terminator
        }
        return result;
    } else {
        // false
        char* result = static_cast<char*>(malloc(6));
        if (result) {
            memcpy(result, "false", 6);  // includes null terminator
        }
        return result;
    }
}

// ============================================
// Boolean.prototype.valueOf()
// Returns the primitive boolean value (0 or 1)
// ============================================
int64_t nova_boolean_valueOf(int64_t value) {
    // Return 0 for false, 1 for true
    return value ? 1 : 0;
}

} // extern "C"
