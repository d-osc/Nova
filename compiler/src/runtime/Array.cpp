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


// Helper: Create metadata struct from ValueArray
void* create_metadata_from_value_array(ValueArray* array) {
    if (!array) return nullptr;
    
    // Allocate metadata struct: { [24 x i8], i64 length, i64 capacity, ptr elements }
    size_t metadata_size = 24 + 8 + 8 + 8;  // header + length + capacity + elements ptr
    void* metadata = malloc(metadata_size);
    if (!metadata) return nullptr;
    
    // Zero out header
    std::memset(metadata, 0, 24);
    
    // Write length, capacity, and elements pointer
    *reinterpret_cast<int64*>(static_cast<char*>(metadata) + 24) = array->length;
    *reinterpret_cast<int64*>(static_cast<char*>(metadata) + 32) = array->capacity;
    *reinterpret_cast<int64**>(static_cast<char*>(metadata) + 40) = array->elements;
    
    return metadata;
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



// Create a new string array with initial capacity
StringArray* create_string_array(int64 initial_capacity) {
    if (initial_capacity < 0) initial_capacity = 0;

    // Allocate StringArray structure
    StringArray* array = static_cast<StringArray*>(allocate(sizeof(StringArray), TypeId::ARRAY));

    // Initialize array
    array->length = 0;
    array->capacity = initial_capacity;

    // Allocate elements array
    if (initial_capacity > 0) {
        size_t elements_size = initial_capacity * sizeof(const char*);
        array->elements = static_cast<const char**>(allocate(elements_size, TypeId::OBJECT));
        // Initialize to null
        for (int64 i = 0; i < initial_capacity; i++) {
            array->elements[i] = nullptr;
        }
    } else {
        array->elements = nullptr;
    }

    return array;
}


// Join array elements into a string with delimiter
const char* value_array_join(ValueArray* array, const char* delimiter) {
    if (!array || array->length == 0) return "";
    if (!delimiter) delimiter = ",";

    size_t delim_len = std::strlen(delimiter);
    
    // Calculate total length needed
    size_t total_len = 0;
    for (int64 i = 0; i < array->length; i++) {
        // Convert int64 to string to get length
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%lld", (long long)array->elements[i]);
        total_len += std::strlen(buffer);
        if (i < array->length - 1) {
            total_len += delim_len;
        }
    }
    
    // Allocate result
    char* result = static_cast<char*>(malloc(total_len + 1));
    if (!result) return "";
    
    // Build the string
    size_t pos = 0;
    for (int64 i = 0; i < array->length; i++) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%lld", (long long)array->elements[i]);
        size_t len = std::strlen(buffer);
        std::memcpy(result + pos, buffer, len);
        pos += len;
        
        if (i < array->length - 1) {
            std::memcpy(result + pos, delimiter, delim_len);
            pos += delim_len;
        }
    }
    result[total_len] = 0;
    
    return result;
}

// Concatenate two arrays into a new array
ValueArray* value_array_concat(ValueArray* arr1, ValueArray* arr2) {
    if (!arr1 && !arr2) return create_value_array(0);
    if (!arr1) {
        // Copy arr2
        ValueArray* result = create_value_array(arr2->length);
        result->length = arr2->length;
        std::memcpy(result->elements, arr2->elements, arr2->length * sizeof(int64));
        return result;
    }
    if (!arr2) {
        // Copy arr1
        ValueArray* result = create_value_array(arr1->length);
        result->length = arr1->length;
        std::memcpy(result->elements, arr1->elements, arr1->length * sizeof(int64));
        return result;
    }
    
    // Create new array with combined length
    int64 total_len = arr1->length + arr2->length;
    ValueArray* result = create_value_array(total_len);
    result->length = total_len;
    
    // Copy elements from both arrays
    std::memcpy(result->elements, arr1->elements, arr1->length * sizeof(int64));
    std::memcpy(result->elements + arr1->length, arr2->elements, arr2->length * sizeof(int64));
    
    return result;
}


// Slice array to create a new array from start to end
ValueArray* value_array_slice(ValueArray* array, int64 start, int64 end) {
    if (!array) return create_value_array(0);
    
    int64 length = array->length;
    
    // Handle negative indices
    if (start < 0) start = std::max(length + start, static_cast<int64>(0));
    if (end < 0) end = std::max(length + end, static_cast<int64>(0));
    
    // Clamp to valid range
    if (start > length) start = length;
    if (end > length) end = length;
    if (start > end) start = end;
    
    int64 slice_len = end - start;
    if (slice_len <= 0) return create_value_array(0);
    
    // Create new array with sliced elements
    ValueArray* result = create_value_array(slice_len);
    result->length = slice_len;
    
    // Copy elements
    std::memcpy(result->elements, array->elements + start, slice_len * sizeof(int64));
    
    return result;
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

const char* nova_value_array_join(void* array_ptr, const char* delimiter) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    return nova::runtime::value_array_join(array, delimiter);
}

void* nova_value_array_concat(void* arr1_ptr, void* arr2_ptr) {
    nova::runtime::ValueArray* arr1 = ensure_value_array(arr1_ptr);
    nova::runtime::ValueArray* arr2 = ensure_value_array(arr2_ptr);
    nova::runtime::ValueArray* result = nova::runtime::value_array_concat(arr1, arr2);
    // Create metadata struct for the new array
    return nova::runtime::create_metadata_from_value_array(result);
}

void* nova_value_array_slice(void* array_ptr, int64_t start, int64_t end) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    nova::runtime::ValueArray* result = nova::runtime::value_array_slice(array, start, end);
    // Create metadata struct for the new array
    return nova::runtime::create_metadata_from_value_array(result);
}

// Callback function type: takes element value, returns boolean (as i64)
typedef int64_t (*FindCallbackFunc)(int64_t);

int64_t nova_value_array_find(void* array_ptr, FindCallbackFunc callback) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback) {
        return 0;  // Not found
    }

    // Iterate through array elements
    for (int64_t i = 0; i < array->length; i++) {
        int64_t element = array->elements[i];

        // Call the callback function
        int64_t result = callback(element);

        // If callback returns truthy value, return this element
        if (result != 0) {
            return element;
        }
    }

    // Not found
    return 0;
}

// Array.findIndex() implementation
// Callback function type: same as find
typedef int64_t (*FindIndexCallbackFunc)(int64_t);

int64_t nova_value_array_findIndex(void* array_ptr, FindIndexCallbackFunc callback) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback) {
        return -1;  // Not found
    }

    // Iterate through array elements
    for (int64_t i = 0; i < array->length; i++) {
        int64_t element = array->elements[i];

        // Call the callback function
        int64_t result = callback(element);

        // If callback returns truthy value, return this index
        if (result != 0) {
            return i;  // Return the INDEX, not the element
        }
    }

    // Not found
    return -1;  // JavaScript standard: return -1 when not found
}

// Array.filter() implementation
// Callback function type: same as find
typedef int64_t (*FilterCallbackFunc)(int64_t);

void* nova_value_array_filter(void* array_ptr, FilterCallbackFunc callback) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback) {
        // Return empty array
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        emptyArray->length = 0;
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    // First pass: count matching elements
    int64_t matchCount = 0;
    for (int64_t i = 0; i < array->length; i++) {
        int64_t element = array->elements[i];
        int64_t result = callback(element);
        if (result != 0) {
            matchCount++;
        }
    }

    // Create new array with exact size needed
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(matchCount);
    resultArray->length = matchCount;

    // Second pass: copy matching elements
    int64_t writeIndex = 0;
    for (int64_t i = 0; i < array->length; i++) {
        int64_t element = array->elements[i];
        int64_t result = callback(element);
        if (result != 0) {
            resultArray->elements[writeIndex++] = element;
        }
    }

    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Array.map() implementation
// Callback function type: takes element, returns transformed value
typedef int64_t (*MapCallbackFunc)(int64_t);

void* nova_value_array_map(void* array_ptr, MapCallbackFunc callback) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback) {
        // Return empty array
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        emptyArray->length = 0;
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    // Create new array with same size as input
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(array->length);
    resultArray->length = array->length;

    // Transform each element
    for (int64_t i = 0; i < array->length; i++) {
        int64_t element = array->elements[i];
        int64_t transformed = callback(element);
        resultArray->elements[i] = transformed;
    }

    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Array.some() implementation
// Callback function type: takes element, returns boolean
typedef int64_t (*SomeCallbackFunc)(int64_t);

int64_t nova_value_array_some(void* array_ptr, SomeCallbackFunc callback) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback) {
        return 0;  // false
    }

    // Check if any element matches
    for (int64_t i = 0; i < array->length; i++) {
        int64_t element = array->elements[i];
        int64_t result = callback(element);

        // If callback returns truthy value, return true immediately
        if (result != 0) {
            return 1;  // true
        }
    }

    return 0;  // false - no elements matched
}

// Array.every() implementation
// Callback function type: takes element, returns boolean
typedef int64_t (*EveryCallbackFunc)(int64_t);

int64_t nova_value_array_every(void* array_ptr, EveryCallbackFunc callback) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback) {
        return 1;  // true - vacuous truth (empty array returns true)
    }

    // Check if all elements match
    for (int64_t i = 0; i < array->length; i++) {
        int64_t element = array->elements[i];
        int64_t result = callback(element);

        // If callback returns falsy value, return false immediately
        if (result == 0) {
            return 0;  // false
        }
    }

    return 1;  // true - all elements matched
}

// Array.forEach() implementation
// Callback function type: takes element, returns value (ignored)
typedef int64_t (*ForEachCallbackFunc)(int64_t);

void nova_value_array_forEach(void* array_ptr, ForEachCallbackFunc callback) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback) {
        return;  // Nothing to do
    }

    // Call callback for each element (return value ignored)
    for (int64_t i = 0; i < array->length; i++) {
        int64_t element = array->elements[i];
        callback(element);  // Call for side effects only
    }
}

// Array.reduce() implementation
// Callback function type: takes accumulator and element, returns new accumulator (2 parameters!)
typedef int64_t (*ReduceCallbackFunc)(int64_t, int64_t);

int64_t nova_value_array_reduce(void* array_ptr, ReduceCallbackFunc callback, int64_t initialValue) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback) {
        return initialValue;  // Return initial value if array or callback is null
    }

    // Start with initial value
    int64_t accumulator = initialValue;

    // Apply callback to each element
    for (int64_t i = 0; i < array->length; i++) {
        int64_t element = array->elements[i];
        accumulator = callback(accumulator, element);  // Update accumulator with callback result
    }

    return accumulator;
}



} // extern "C"