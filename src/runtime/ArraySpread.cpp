#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cstdio>

// Forward declare the functions from Array.cpp
extern "C" {
    void* create_value_array(int64_t initial_capacity);
    int64_t value_array_length(void* array_ptr);
    int64_t value_array_get(void* array_ptr, int64_t index);
    void value_array_set(void* array_ptr, int64_t index, int64_t value);
}

// Internal structure matching ValueArray in Array.cpp
struct ValueArrayHeader {
    char object_header[24];  // ObjectHeader
    int64_t length;
    int64_t capacity;
    int64_t* elements;
};

extern "C" {

// Set array length (used by complex spread operator)
void nova_array_set_length(void* array_ptr, int64_t new_length) {
    if (!array_ptr) return;
    ValueArrayHeader* array = static_cast<ValueArrayHeader*>(array_ptr);
    array->length = new_length;
}

// Copy an array (for spread operator)
// Returns a new array with the same elements
void* nova_array_copy(void* source_array_ptr) {
    if (!source_array_ptr) return nullptr;

    int64_t length = value_array_length(source_array_ptr);

    void* result = create_value_array(length);
    if (!result) return nullptr;

    // Set length before copying (value_array_set checks index < length)
    ValueArrayHeader* result_array = static_cast<ValueArrayHeader*>(result);
    result_array->length = length;

    // Copy all elements
    for (int64_t i = 0; i < length; i++) {
        int64_t value = value_array_get(source_array_ptr, i);
        value_array_set(result, i, value);
    }

    return result;
}

} // extern "C"
