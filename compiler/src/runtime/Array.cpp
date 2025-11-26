#include "nova/runtime/Runtime.h"
#include <cstring>
#include <algorithm>

namespace nova {
namespace runtime {

Array* create_array(int64 initial_capacity) {
    if (initial_capacity < 0) {
        initial_capacity = 8;
    }
    
    // Allocate array structure
    Array* array = static_cast<Array*>(allocate(sizeof(Array), TypeId::ARRAY));
    
    // Initialize array
    array->length = 0;
    array->capacity = initial_capacity;
    
    // Allocate elements array
    size_t elements_size = initial_capacity * sizeof(void*);
    array->elements = allocate(elements_size, TypeId::OBJECT);
    
    return array;
}

void resize_array(Array* array, int64 new_capacity) {
    if (!array || new_capacity < array->length) return;
    
    // Allocate new elements array
    size_t new_elements_size = new_capacity * sizeof(void*);
    void* new_elements = allocate(new_elements_size, TypeId::OBJECT);
    
    // Copy existing elements
    if (array->elements && array->length > 0) {
        std::memcpy(new_elements, array->elements, array->length * sizeof(void*));
        deallocate(array->elements);
    }
    
    // Update array
    array->elements = new_elements;
    array->capacity = new_capacity;
}

void* array_get(Array* array, int64 index) {
    if (!array || index < 0 || index >= array->length) return nullptr;
    
    void** elements = static_cast<void**>(array->elements);
    return elements[index];
}

void array_set(Array* array, int64 index, void* value) {
    if (!array || index < 0 || index >= array->length) return;
    
    void** elements = static_cast<void**>(array->elements);
    elements[index] = value;
}

int64 array_length(Array* array) {
    return array ? array->length : 0;
}

void array_push(Array* array, void* value) {
    if (!array) return;
    
    // Resize if needed
    if (array->length >= array->capacity) {
        int64 new_capacity = array->capacity * 2;
        if (new_capacity < 8) new_capacity = 8;
        resize_array(array, new_capacity);
    }
    
    // Add element
    void** elements = static_cast<void**>(array->elements);
    elements[array->length] = value;
    array->length++;
}

void* array_pop(Array* array) {
    if (!array || array->length <= 0) return nullptr;

    array->length--;
    void** elements = static_cast<void**>(array->elements);
    return elements[array->length];
}

void* array_shift(Array* array) {
    if (!array || array->length <= 0) return nullptr;

    void** elements = static_cast<void**>(array->elements);
    void* first = elements[0];

    // Shift all elements down by one
    for (int64 i = 1; i < array->length; i++) {
        elements[i - 1] = elements[i];
    }

    array->length--;
    return first;
}

void array_unshift(Array* array, void* value) {
    if (!array) return;

    // Resize if needed
    if (array->length >= array->capacity) {
        int64 new_capacity = array->capacity * 2;
        if (new_capacity < 8) new_capacity = 8;
        resize_array(array, new_capacity);
    }

    void** elements = static_cast<void**>(array->elements);

    // Shift all elements up by one
    for (int64 i = array->length; i > 0; i--) {
        elements[i] = elements[i - 1];
    }

    // Insert new element at beginning
    elements[0] = value;
    array->length++;
}

// ==================== Value Array Functions ====================
// These functions work with value-based arrays (int64 elements)
// Used for primitive type arrays like number[]

ValueArray* create_value_array(int64 initial_capacity) {
    if (initial_capacity < 1) initial_capacity = 8;

    ValueArray* array = static_cast<ValueArray*>(allocate(sizeof(ValueArray), TypeId::ARRAY));
    array->length = 0;
    array->capacity = initial_capacity;

    // Allocate int64 array for direct value storage
    array->elements = static_cast<int64*>(malloc(sizeof(int64) * initial_capacity));

    add_root(array);
    return array;
}

// Convert stack-based array metadata to heap-based ValueArray
// Metadata struct format: { [24 x i8], i64 length, i64 capacity, ptr elements }
ValueArray* convert_to_value_array(void* metadata_ptr) {
    if (!metadata_ptr) return nullptr;

    // Read length and elements pointer from metadata struct
    // Offset 24 bytes for ObjectHeader placeholder, then i64 length, i64 capacity, ptr elements
    int64 length = *reinterpret_cast<int64*>(static_cast<char*>(metadata_ptr) + 24);
    int64 capacity = *reinterpret_cast<int64*>(static_cast<char*>(metadata_ptr) + 32);
    int64* stack_elements = *reinterpret_cast<int64**>(static_cast<char*>(metadata_ptr) + 40);

    // Create a new ValueArray
    ValueArray* array = create_value_array(length > capacity ? length : capacity);
    array->length = length;

    // Copy values from stack array to heap array
    if (stack_elements && length > 0) {
        for (int64 i = 0; i < length; i++) {
            array->elements[i] = stack_elements[i];
        }
    }

    return array;
}

void resize_value_array(ValueArray* array, int64 new_capacity) {
    if (!array) return;

    int64* new_elements = static_cast<int64*>(malloc(sizeof(int64) * new_capacity));

    // Copy existing elements
    int64 copy_count = (array->length < new_capacity) ? array->length : new_capacity;
    for (int64 i = 0; i < copy_count; i++) {
        new_elements[i] = array->elements[i];
    }

    free(array->elements);
    array->elements = new_elements;
    array->capacity = new_capacity;
}

int64 value_array_get(ValueArray* array, int64 index) {
    if (!array || index < 0 || index >= array->length) {
        return 0;  // Return 0 for out of bounds
    }
    return array->elements[index];
}

void value_array_set(ValueArray* array, int64 index, int64 value) {
    if (!array || index < 0 || index >= array->length) {
        return;
    }
    array->elements[index] = value;
}

int64 value_array_length(ValueArray* array) {
    if (!array) return 0;
    return array->length;
}

void value_array_push(ValueArray* array, int64 value) {
    if (!array) return;

    // Resize if needed
    if (array->length >= array->capacity) {
        int64 new_capacity = array->capacity * 2;
        if (new_capacity < 8) new_capacity = 8;
        resize_value_array(array, new_capacity);
    }

    array->elements[array->length] = value;
    array->length++;
}

int64 value_array_pop(ValueArray* array) {
    if (!array || array->length <= 0) {
        return 0;  // Return 0 for empty array
    }

    array->length--;
    return array->elements[array->length];
}

int64 value_array_shift(ValueArray* array) {
    if (!array || array->length <= 0) {
        return 0;
    }

    int64 first = array->elements[0];

    // Shift all elements down by one
    for (int64 i = 1; i < array->length; i++) {
        array->elements[i - 1] = array->elements[i];
    }

    array->length--;
    return first;
}

void value_array_unshift(ValueArray* array, int64 value) {
    if (!array) return;

    // Resize if needed
    if (array->length >= array->capacity) {
        int64 new_capacity = array->capacity * 2;
        if (new_capacity < 8) new_capacity = 8;
        resize_value_array(array, new_capacity);
    }

    // Shift all elements up by one
    for (int64 i = array->length; i > 0; i--) {
        array->elements[i] = array->elements[i - 1];
    }

    // Insert new element at beginning
    array->elements[0] = value;
    array->length++;
}

// Check if array includes a value
bool value_array_includes(ValueArray* array, int64 value) {
    if (!array) return false;

    for (int64 i = 0; i < array->length; i++) {
        if (array->elements[i] == value) {
            return true;
        }
    }
    return false;
}

// Find index of a value in array
int64 value_array_indexOf(ValueArray* array, int64 value) {
    if (!array) return -1;

    for (int64 i = 0; i < array->length; i++) {
        if (array->elements[i] == value) {
            return i;
        }
    }
    return -1;  // Not found
}

// Reverse array in place
void value_array_reverse(ValueArray* array) {
    if (!array || array->length <= 1) return;

    int64 left = 0;
    int64 right = array->length - 1;

    while (left < right) {
        // Swap elements
        int64 temp = array->elements[left];
        array->elements[left] = array->elements[right];
        array->elements[right] = temp;

        left++;
        right--;
    }
}

// Fill array with a static value
void value_array_fill(ValueArray* array, int64 value) {
    if (!array) return;

    for (int64 i = 0; i < array->length; i++) {
        array->elements[i] = value;
    }
}

} // namespace runtime
} // namespace nova

// Extern "C" wrappers for array methods (for easier linking)
extern "C" {

nova::runtime::ValueArray* nova_convert_to_value_array(void* metadata_ptr) {
    return nova::runtime::convert_to_value_array(metadata_ptr);
}

void* nova_array_push(nova::runtime::Array* array, void* value) {
    nova::runtime::array_push(array, value);
    return nullptr;  // push returns void, but we return nullptr for consistency
}

void* nova_array_pop(nova::runtime::Array* array) {
    return nova::runtime::array_pop(array);
}

void* nova_array_shift(nova::runtime::Array* array) {
    return nova::runtime::array_shift(array);
}

void* nova_array_unshift(nova::runtime::Array* array, void* value) {
    nova::runtime::array_unshift(array, value);
    return nullptr;  // unshift returns void, but we return nullptr for consistency
}

// Value array wrappers with automatic metadata conversion
// These accept metadata struct pointers and convert them to ValueArray on first use

// Cache to store converted ValueArrays to avoid reconversion
#include <unordered_map>
static std::unordered_map<void*, nova::runtime::ValueArray*> conversion_cache;

// Helper to check if a pointer is already a ValueArray or needs conversion
static nova::runtime::ValueArray* ensure_value_array(void* ptr) {
    if (!ptr) return nullptr;

    // Check if already converted
    auto it = conversion_cache.find(ptr);
    if (it != conversion_cache.end()) {
        return it->second;
    }

    // Convert from metadata struct to ValueArray
    nova::runtime::ValueArray* array = nova::runtime::convert_to_value_array(ptr);

    // Cache the conversion
    conversion_cache[ptr] = array;

    return array;
}

// Helper to write ValueArray data back to metadata struct
// This ensures array indexing sees the modified data
static void write_back_to_metadata(void* metadata_ptr, nova::runtime::ValueArray* array) {
    if (!metadata_ptr || !array) return;

    // Update length in metadata struct (offset 24)
    *reinterpret_cast<int64_t*>(static_cast<char*>(metadata_ptr) + 24) = array->length;
    // Update capacity in metadata struct (offset 32)
    *reinterpret_cast<int64_t*>(static_cast<char*>(metadata_ptr) + 32) = array->capacity;
    // Update elements pointer in metadata struct (offset 40) to point to ValueArray's elements
    *reinterpret_cast<int64_t**>(static_cast<char*>(metadata_ptr) + 40) = array->elements;
}

void nova_value_array_push(void* array_ptr, int64_t value) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    nova::runtime::value_array_push(array, value);
    write_back_to_metadata(array_ptr, array);
}

int64_t nova_value_array_pop(void* array_ptr) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    int64_t result = nova::runtime::value_array_pop(array);
    write_back_to_metadata(array_ptr, array);
    return result;
}

int64_t nova_value_array_shift(void* array_ptr) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    int64_t result = nova::runtime::value_array_shift(array);
    write_back_to_metadata(array_ptr, array);
    return result;
}

void nova_value_array_unshift(void* array_ptr, int64_t value) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    nova::runtime::value_array_unshift(array, value);
    write_back_to_metadata(array_ptr, array);
}

int64_t nova_value_array_includes(void* array_ptr, int64_t value) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    bool result = nova::runtime::value_array_includes(array, value);
    return result ? 1 : 0;  // Convert bool to int64 (1 for true, 0 for false)
}

int64_t nova_value_array_indexOf(void* array_ptr, int64_t value) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    return nova::runtime::value_array_indexOf(array, value);
}

void* nova_value_array_reverse(void* array_ptr) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    nova::runtime::value_array_reverse(array);
    write_back_to_metadata(array_ptr, array);
    return array_ptr;  // Return array for chaining (like JavaScript)
}

void* nova_value_array_fill(void* array_ptr, int64_t value) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    nova::runtime::value_array_fill(array, value);
    write_back_to_metadata(array_ptr, array);
    return array_ptr;  // Return array for chaining (like JavaScript)
}

} // extern "C"