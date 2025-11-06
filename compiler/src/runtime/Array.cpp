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

} // namespace runtime
} // namespace nova