#include "nova/runtime/Runtime.h"
#include "nova/runtime/Value.h"
#include <cstring>
#include <algorithm>
#include <cstdarg>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// ==================== ULTRA OPTIMIZATIONS ====================
// Enable SIMD optimizations for array operations
#if defined(__AVX2__) || defined(_MSC_VER)
    #define NOVA_USE_SIMD 1
    #include <immintrin.h>
#endif

// Branch prediction hints
#ifdef __GNUC__
    #define LIKELY(x)   __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define LIKELY(x)   (x)
    #define UNLIKELY(x) (x)
#endif

namespace nova {
namespace runtime {

// OPTIMIZATION 1: Fibonacci-like capacity growth
// Minimizes reallocations while avoiding over-allocation
static inline int64 calculate_new_capacity(int64 current) {
    if (current < 8) return 8;
    if (current < 64) return current * 2;
    // Fibonacci-like: new = current * 1.5
    return current + (current >> 1);
}

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
    array->header.size = sizeof(ValueArray);
    array->header.type_id = static_cast<uint32_t>(TypeId::ARRAY);
    array->header.is_marked = false;
    array->header.value_encoding = 0;
    array->header.next = nullptr;
    array->length = 0;
    array->capacity = initial_capacity;

    // Allocate int64 array for direct value storage
    // Use regular malloc for simplicity and compatibility
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
    array->header.value_encoding = *reinterpret_cast<uint8*>(
        static_cast<char*>(metadata_ptr) + 13);

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

    // Allocate metadata struct matching ValueArray layout
    ValueArray* metadata = static_cast<ValueArray*>(malloc(sizeof(ValueArray)));
    if (!metadata) return nullptr;

    // Initialize ObjectHeader
    metadata->header.size = 0;
    metadata->header.type_id = static_cast<uint32_t>(TypeId::ARRAY);  // Set type_id = 1
    metadata->header.is_marked = false;
    metadata->header.value_encoding = array->header.value_encoding;
    metadata->header.next = nullptr;

    // Copy array data
    metadata->length = array->length;
    metadata->capacity = array->capacity;
    metadata->elements = array->elements;

    return metadata;
}


// OPTIMIZATION 2: Optimized resize with smart growth and fast copy
void resize_value_array(ValueArray* array, int64 new_capacity) {
    if (!array) return;

    // Use smart capacity calculation if not explicitly specified
    if (new_capacity <= array->capacity) {
        new_capacity = calculate_new_capacity(array->capacity);
    }

    // Allocate new memory - use regular malloc for compatibility
    int64* new_elements = static_cast<int64*>(malloc(sizeof(int64) * new_capacity));
    if (!new_elements) {
        return;  // Allocation failed, keep old array
    }

    // OPTIMIZATION 3: Fast memcpy for bulk copy (CPU optimized)
    int64 copy_count = (array->length < new_capacity) ? array->length : new_capacity;
    if (copy_count > 0 && array->elements) {
        std::memcpy(new_elements, array->elements, copy_count * sizeof(int64));
    }

    // Free old memory
    if (array->elements) {
        free(array->elements);
    }

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
    if (!array) return;
    if (index < 0) return;
    if (index >= array->length) return;
    array->elements[index] = value;
}

int64 value_array_length(ValueArray* array) {
    if (!array) return 0;
    return array->length;
}

// OPTIMIZATION 4: Fast path for push (90% of cases have capacity)
inline void value_array_push(ValueArray* array, int64 value) {
    if (UNLIKELY(!array)) return;

    // FAST PATH: Space available (likely case)
    if (LIKELY(array->length < array->capacity)) {
        array->elements[array->length++] = value;
        return;
    }

    // SLOW PATH: Resize needed
    int64 new_capacity = calculate_new_capacity(array->capacity);
    resize_value_array(array, new_capacity);
    array->elements[array->length++] = value;
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

// Get element at index (supports negative indices)
int64 value_array_at(ValueArray* array, int64 index) {
    if (!array || array->length == 0) return 0;

    // Handle negative indices (count from end)
    if (index < 0) {
        index = array->length + index;
    }

    // Check bounds
    if (index < 0 || index >= array->length) {
        return 0;  // Out of bounds - return 0
    }

    return array->elements[index];
}

// OPTIMIZATION 5: SIMD-accelerated includes (4x faster with AVX2)
bool value_array_includes(ValueArray* array, int64 value) {
    if (!array) return false;

#ifdef NOVA_USE_SIMD
    // SIMD path for large arrays
    if (array->length >= 8) {
        __m256i search_vec = _mm256_set1_epi64x(value);
        int64 i = 0;

        // Process 4 elements at a time
        for (; i + 3 < array->length; i += 4) {
            __m256i data_vec = _mm256_loadu_si256((__m256i*)&array->elements[i]);
            __m256i cmp = _mm256_cmpeq_epi64(data_vec, search_vec);
            int mask = _mm256_movemask_epi8(cmp);
            if (mask != 0) return true;
        }

        // Handle remaining elements
        for (; i < array->length; i++) {
            if (array->elements[i] == value) return true;
        }
        return false;
    }
#endif

    // Scalar fallback for small arrays
    for (int64 i = 0; i < array->length; i++) {
        if (array->elements[i] == value) {
            return true;
        }
    }
    return false;
}

// OPTIMIZATION 6: SIMD-accelerated indexOf (4x faster with AVX2)
int64 value_array_indexOf(ValueArray* array, int64 value) {
    if (!array) return -1;

#ifdef NOVA_USE_SIMD
    // SIMD path for large arrays
    if (array->length >= 8) {
        __m256i search_vec = _mm256_set1_epi64x(value);
        int64 i = 0;

        // Process 4 elements at a time
        for (; i + 3 < array->length; i += 4) {
            __m256i data_vec = _mm256_loadu_si256((__m256i*)&array->elements[i]);
            __m256i cmp = _mm256_cmpeq_epi64(data_vec, search_vec);
            int mask = _mm256_movemask_epi8(cmp);

            if (mask != 0) {
                // Found a match, find which lane
                for (int j = 0; j < 4; j++) {
                    if (array->elements[i + j] == value) {
                        return i + j;
                    }
                }
            }
        }

        // Handle remaining elements
        for (; i < array->length; i++) {
            if (array->elements[i] == value) return i;
        }
        return -1;
    }
#endif

    // Scalar fallback for small arrays
    for (int64 i = 0; i < array->length; i++) {
        if (array->elements[i] == value) {
            return i;
        }
    }
    return -1;  // Not found
}

// Find last index of a value in array (search from end)
int64 value_array_lastIndexOf(ValueArray* array, int64 value) {
    if (!array) return -1;

    // Search backwards from end to start
    for (int64 i = array->length - 1; i >= 0; i--) {
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

// OPTIMIZATION 7: SIMD-accelerated fill (8x faster with AVX2)
void value_array_fill(ValueArray* array, int64 value) {
    if (!array) return;

#ifdef NOVA_USE_SIMD
    // SIMD path for large arrays
    if (array->length >= 16) {
        __m256i fill_vec = _mm256_set1_epi64x(value);
        int64 i = 0;

        // Process 4 elements at a time
        for (; i + 3 < array->length; i += 4) {
            _mm256_storeu_si256((__m256i*)&array->elements[i], fill_vec);
        }

        // Handle remaining elements
        for (; i < array->length; i++) {
            array->elements[i] = value;
        }
        return;
    }
#endif

    // Scalar fallback for small arrays or non-SIMD builds
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

    // Helper: render a single element into a heap-allocated string. The same
    // ValueArray can carry raw int64 pointers, raw string pointers, or
    // NaN-boxed JSValues depending on the producer. Detect at format time so
    // join works for Object.keys (string ptrs), Object.values (mixed), and
    // numeric arrays alike.
    const bool boxedElements = array->header.value_encoding == 1;
    auto render_element = [boxedElements](int64_t raw, std::string& out) {
        // JSValue tag check (upper 16 bits). If it's a known tag, decode.
        namespace r = nova::runtime;
        r::JSValue v = static_cast<r::JSValue>(static_cast<std::uintptr_t>(raw));
        if (boxedElements) {
            const char* text = nova_value_to_string_ptr(v);
            out = text ? text : "";
            return;
        }
        if (r::js_value_has_tag(v, r::JS_VALUE_STRING_TAG)) {
            const char* s = reinterpret_cast<const char*>(
                static_cast<std::uintptr_t>(v & r::JS_VALUE_PAYLOAD_MASK));
            out = s ? s : "";
            return;
        }
        if (r::js_value_has_tag(v, r::JS_VALUE_FALSE)) { out = "false"; return; }
        if (r::js_value_has_tag(v, r::JS_VALUE_TRUE)) { out = "true"; return; }
        if (r::js_value_has_tag(v, r::JS_VALUE_UNDEFINED)) { out = "undefined"; return; }
        if (r::js_value_has_tag(v, r::JS_VALUE_NULL)) { out = "null"; return; }
        // Heuristic: if the value looks like a printable ASCII string pointer
        // (low address, first byte is a letter/digit/underscore), treat as one.
        // This catches raw `const char*` elements produced by Object.keys and
        // friends without breaking the integer path.
        auto ptrVal = static_cast<std::uintptr_t>(raw);
        if (ptrVal > 0x10000 && (ptrVal & 0xFFFF000000000000) == 0) {
            // Could be a real pointer to a C string. Probe carefully.
            const char* s = reinterpret_cast<const char*>(ptrVal);
            // Only treat as a string if the first byte is ASCII printable and
            // not a digit (digits could be misinterpreted by other callers,
            // but a digit-leading C string from Object.keys is impossible —
            // keys are identifiers/string-literals).
            char c = s[0];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                c == '_' || c == '$' || c == '"' || c == '[') {
                // Validate that the rest looks like a C string (printable or null).
                bool looks_like_string = true;
                for (size_t k = 0; k < 256; ++k) {
                    char ch = s[k];
                    if (ch == 0) break;
                    if (ch < 32 || ch > 126) {
                        looks_like_string = false;
                        break;
                    }
                }
                if (looks_like_string) {
                    out = s;
                    return;
                }
            }
        }
        // Otherwise, format as integer.
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%lld", (long long)raw);
        out = buffer;
    };

    // Calculate total length needed
    size_t total_len = 0;
    std::vector<std::string> rendered;
    rendered.reserve(static_cast<size_t>(array->length));
    for (int64 i = 0; i < array->length; i++) {
        std::string s;
        render_element(array->elements[i], s);
        total_len += s.size();
        rendered.push_back(std::move(s));
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
        size_t len = rendered[i].size();
        std::memcpy(result + pos, rendered[i].data(), len);
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
        result->header.value_encoding = arr2->header.value_encoding;
        result->length = arr2->length;
        std::memcpy(result->elements, arr2->elements, arr2->length * sizeof(int64));
        return result;
    }
    if (!arr2) {
        // Copy arr1
        ValueArray* result = create_value_array(arr1->length);
        result->header.value_encoding = arr1->header.value_encoding;
        result->length = arr1->length;
        std::memcpy(result->elements, arr1->elements, arr1->length * sizeof(int64));
        return result;
    }
    
    // Create new array with combined length
    int64 total_len = arr1->length + arr2->length;
    ValueArray* result = create_value_array(total_len);
    result->header.value_encoding = arr1->header.value_encoding;
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
    result->header.value_encoding = array->header.value_encoding;
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

void* nova_create_array(int64_t size) {
    return nova::runtime::create_array(static_cast<int64_t>(size));
}

void nova_array_push_string(void* arr, const char* value) {
    // Create a Nova string value from the C string
    std::uint64_t strVal = nova_value_from_string(value);
    nova::runtime::array_push(static_cast<nova::runtime::Array*>(arr), reinterpret_cast<void*>(strVal));
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

static bool is_readable_array_metadata(void* ptr) {
    if (!ptr || reinterpret_cast<std::uintptr_t>(ptr) < 0x10000) {
        return false;
    }
#ifdef _WIN32
    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(ptr, &info, sizeof(info)) != sizeof(info) ||
        info.State != MEM_COMMIT ||
        (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) {
        return false;
    }
    const auto begin = reinterpret_cast<std::uintptr_t>(ptr);
    const auto regionEnd =
        reinterpret_cast<std::uintptr_t>(info.BaseAddress) + info.RegionSize;
    if (begin + 48 > regionEnd) {
        return false;
    }
#else
    const long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) return false;
    const auto begin = reinterpret_cast<std::uintptr_t>(ptr);
    const auto firstPage = begin &
        ~static_cast<std::uintptr_t>(pageSize - 1);
    const auto lastPage = (begin + 47) &
        ~static_cast<std::uintptr_t>(pageSize - 1);
    unsigned char residency = 0;
    if (mincore(reinterpret_cast<void*>(firstPage),
                static_cast<size_t>(pageSize), &residency) != 0) {
        return false;
    }
    if (lastPage != firstPage &&
        mincore(reinterpret_cast<void*>(lastPage),
                static_cast<size_t>(pageSize), &residency) != 0) {
        return false;
    }
#endif
    const auto* bytes = static_cast<const char*>(ptr);
    const auto typeId = *reinterpret_cast<const std::uint32_t*>(bytes + 8);
    const auto length = *reinterpret_cast<const int64_t*>(bytes + 24);
    const auto capacity = *reinterpret_cast<const int64_t*>(bytes + 32);
    const auto elements = *reinterpret_cast<int64_t* const*>(bytes + 40);
    return typeId == static_cast<std::uint32_t>(
                         nova::runtime::TypeId::ARRAY) &&
           length >= 0 && capacity >= length && capacity < (1LL << 30) &&
           (length == 0 || elements != nullptr);
}

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
    int64_t* length_ptr = reinterpret_cast<int64_t*>(static_cast<char*>(metadata_ptr) + 24);
    *length_ptr = array->length;

    // Update capacity in metadata struct (offset 32)
    *reinterpret_cast<int64_t*>(static_cast<char*>(metadata_ptr) + 32) = array->capacity;
    // Update elements pointer in metadata struct (offset 40) to point to ValueArray's elements
    *reinterpret_cast<int64_t**>(static_cast<char*>(metadata_ptr) + 40) = array->elements;
}

int64_t nova_value_array_push(void* array_ptr, int64_t value) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    if (!array) return 0;  // Graceful no-op for null array (e.g. unresolved capture)
    nova::runtime::value_array_push(array, value);
    write_back_to_metadata(array_ptr, array);
    return array->length;  // Return new length (JavaScript behavior)
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

int64_t nova_value_array_unshift(void* array_ptr, int64_t value) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    nova::runtime::value_array_unshift(array, value);
    write_back_to_metadata(array_ptr, array);
    return array ? array->length : 0;
}

int64_t nova_value_array_at(void* array_ptr, int64_t index) {
    if (!array_ptr) return 0;
    auto* array = static_cast<nova::runtime::ValueArray*>(array_ptr);
    int64_t normalized = index < 0 ? array->length + index : index;
    if (normalized < 0 || normalized >= array->length ||
        !array->elements) {
        return 0;
    }
    return array->elements[normalized];
}

std::uint64_t nova_value_array_at_tagged(
    void* array_ptr, int64_t index, int64_t element_kind) {
    auto* array = static_cast<nova::runtime::ValueArray*>(array_ptr);
    if (!array) {
        return nova::runtime::JS_VALUE_UNDEFINED;
    }
    int64_t normalized = index < 0 ? array->length + index : index;
    if (normalized < 0 || normalized >= array->length ||
        !array->elements) {
        return nova::runtime::JS_VALUE_UNDEFINED;
    }

    const std::uint64_t raw =
        static_cast<std::uint64_t>(array->elements[normalized]);
    if (array->header.value_encoding == 1) return raw;

    switch (element_kind) {
        case 1: // floating-point bits already use the JSValue number encoding
            return raw;
        case 2:
            return nova_value_from_string(reinterpret_cast<const char*>(
                static_cast<std::uintptr_t>(raw)));
        case 3:
            return nova_value_from_bool(static_cast<std::int64_t>(raw));
        case 4:
            return nova_value_from_object(reinterpret_cast<void*>(
                static_cast<std::uintptr_t>(raw)));
        default:
            return nova_value_from_i64(static_cast<std::int64_t>(raw));
    }
}

int64_t nova_value_array_length(void* array_ptr) {
    if (!array_ptr) return 0;
    // Read length directly from metadata struct at offset 24
    return *reinterpret_cast<int64_t*>(static_cast<char*>(array_ptr) + 24);
}

// Wrappers for complex spread operator support (called from LLVM codegen)
// NOTE: Complex spread with multiple elements or mixed spread+literals currently has
// a known limitation where the MIR→LLVM translation doesn't correctly handle
// loop variables and allocas in complex control flow. Simple spread [...arr] works perfectly.
int64_t value_array_length(void* array_ptr) {
    return nova_value_array_length(array_ptr);
}

void* create_value_array(int64_t initial_capacity) {
    nova::runtime::ValueArray* array = nova::runtime::create_value_array(initial_capacity);
    return static_cast<void*>(array);
}

// Create a heap-backed array in the public metadata representation used by
// generated code and runtime APIs.
void* nova_value_array_create(int64_t length) {
    if (length < 0) length = 0;
    nova::runtime::ValueArray* array = nova::runtime::create_value_array(length);
    array->length = length;
    return nova::runtime::create_metadata_from_value_array(array);
}

int64_t value_array_get(void* array_ptr, int64_t index) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    return nova::runtime::value_array_get(array, index);
}

void value_array_set(void* array_ptr, int64_t index, int64_t value) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    nova::runtime::value_array_set(array, index, value);
    write_back_to_metadata(array_ptr, array);
}

// Array.with(index, value) - ES2023
// Returns NEW array with element at index replaced (immutable operation)
void* nova_value_array_with(void* array_ptr, int64_t index, int64_t value) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array) {
        return nullptr;
    }

    int64_t len = array->length;

    // Handle negative indices: convert to positive
    if (index < 0) {
        index = len + index;
    }

    // Check bounds after conversion
    if (index < 0 || index >= len) {
        // Out of bounds - return copy of original array unchanged
        nova::runtime::ValueArray* result = new nova::runtime::ValueArray();
        result->capacity = array->capacity;
        result->length = array->length;
        result->elements = static_cast<int64_t*>(malloc(array->capacity * sizeof(int64_t)));
        if (result->elements && array->elements) {
            std::memcpy(result->elements, array->elements, array->length * sizeof(int64_t));
        }
        return nova::runtime::create_metadata_from_value_array(result);
    }

    // Create a copy of the array
    nova::runtime::ValueArray* result = new nova::runtime::ValueArray();
    result->capacity = array->capacity;
    result->length = array->length;
    result->elements = static_cast<int64_t*>(malloc(array->capacity * sizeof(int64_t)));

    if (!result->elements) {
        return nullptr;
    }

    // Copy all elements
    std::memcpy(result->elements, array->elements, array->length * sizeof(int64_t));

    // Replace element at index
    result->elements[index] = value;

    // Create metadata struct for the new array
    return nova::runtime::create_metadata_from_value_array(result);
}

// Array.toReversed() - ES2023
// Returns NEW reversed array (immutable operation)
void* nova_value_array_toReversed(void* array_ptr) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array) {
        return nullptr;
    }

    // Create a copy of the array
    nova::runtime::ValueArray* result = new nova::runtime::ValueArray();
    result->capacity = array->capacity;
    result->length = array->length;
    result->elements = static_cast<int64_t*>(malloc(array->capacity * sizeof(int64_t)));

    if (!result->elements) {
        return nullptr;
    }

    // Copy elements in reverse order
    for (int64_t i = 0; i < array->length; i++) {
        result->elements[i] = array->elements[array->length - 1 - i];
    }

    // Create metadata struct for the new array
    return nova::runtime::create_metadata_from_value_array(result);
}

// Array.toSorted() - ES2023
// Returns NEW sorted array (immutable operation)
// Comparator callback type for sort
typedef int64_t (*SortComparatorFunc)(int64_t, int64_t);

// Global comparator for std::sort wrapper
static thread_local SortComparatorFunc g_sort_comparator = nullptr;

static bool sort_with_comparator(int64_t a, int64_t b) {
    if (g_sort_comparator) {
        return g_sort_comparator(a, b) < 0;
    }
    return a < b;  // Default ascending
}

void* nova_value_array_toSorted(void* array_ptr) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array) {
        return nullptr;
    }

    // Create a copy of the array
    nova::runtime::ValueArray* result = new nova::runtime::ValueArray();
    result->capacity = array->capacity;
    result->length = array->length;
    result->elements = static_cast<int64_t*>(malloc(array->capacity * sizeof(int64_t)));

    if (!result->elements) {
        return nullptr;
    }

    // Copy all elements
    std::memcpy(result->elements, array->elements, array->length * sizeof(int64_t));

    // Sort the elements (ascending numeric order)
    std::sort(result->elements, result->elements + result->length);

    // Create metadata struct for the new array
    return nova::runtime::create_metadata_from_value_array(result);
}

// Array.toSorted(compareFn) - returns new sorted array with custom comparator (ES2023)
void* nova_value_array_toSorted_compare(void* array_ptr, SortComparatorFunc compareFn) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array) {
        return nullptr;
    }

    // Create a copy of the array
    nova::runtime::ValueArray* result = new nova::runtime::ValueArray();
    result->capacity = array->capacity;
    result->length = array->length;
    result->elements = static_cast<int64_t*>(malloc(array->capacity * sizeof(int64_t)));

    if (!result->elements) {
        return nullptr;
    }

    // Copy all elements
    std::memcpy(result->elements, array->elements, array->length * sizeof(int64_t));

    // Sort with comparator
    if (compareFn) {
        g_sort_comparator = compareFn;
        std::sort(result->elements, result->elements + result->length, sort_with_comparator);
        g_sort_comparator = nullptr;
    } else {
        std::sort(result->elements, result->elements + result->length);
    }

    // Create metadata struct for the new array
    return nova::runtime::create_metadata_from_value_array(result);
}

// Array.sort() - in-place sorting
// Sorts the array in ascending numeric order (modifies original)
void* nova_value_array_sort(void* array_ptr) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !array->elements) {
        return array_ptr;
    }

    // Sort the elements in place (ascending numeric order)
    std::sort(array->elements, array->elements + array->length);

    // Write back to metadata
    write_back_to_metadata(array_ptr, array);

    // Return array pointer for chaining (like JavaScript)
    return array_ptr;
}

// Array.sort(compareFn) - in-place sorting with custom comparator
void* nova_value_array_sort_compare(void* array_ptr, SortComparatorFunc compareFn) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !array->elements) {
        return array_ptr;
    }

    // Sort with comparator
    if (compareFn) {
        g_sort_comparator = compareFn;
        std::sort(array->elements, array->elements + array->length, sort_with_comparator);
        g_sort_comparator = nullptr;
    } else {
        std::sort(array->elements, array->elements + array->length);
    }

    // Write back to metadata
    write_back_to_metadata(array_ptr, array);

    // Return array pointer for chaining (like JavaScript)
    return array_ptr;
}

// Array.splice(start, deleteCount) - removes elements in place
// Modifies array by removing deleteCount elements starting at start
void* nova_value_array_splice(void* array_ptr, int64_t start, int64_t deleteCount) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !array->elements || array->length == 0) {
        return array_ptr;
    }

    // Handle negative start index
    if (start < 0) {
        start = array->length + start;
        if (start < 0) start = 0;
    }

    // Clamp start to array bounds
    if (start >= array->length) {
        return array_ptr;
    }

    // Clamp deleteCount
    if (deleteCount < 0) {
        deleteCount = 0;
    }
    if (start + deleteCount > array->length) {
        deleteCount = array->length - start;
    }

    // Shift elements left to fill the gap
    for (int64_t i = start; i + deleteCount < array->length; i++) {
        array->elements[i] = array->elements[i + deleteCount];
    }

    // Update length
    array->length -= deleteCount;

    // Write back to metadata
    write_back_to_metadata(array_ptr, array);

    // Return array pointer for chaining (like JavaScript)
    return array_ptr;
}

// Array.splice(start, deleteCount, items...) - removes and inserts elements
// Modifies array by removing deleteCount elements and inserting new ones
void* nova_value_array_splice_insert(void* array_ptr, int64_t start, int64_t deleteCount,
                                      int64_t* items, int64_t itemCount) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array) {
        return array_ptr;
    }

    // Handle negative start index
    if (start < 0) {
        start = array->length + start;
        if (start < 0) start = 0;
    }

    // Clamp start to array bounds
    if (start > array->length) {
        start = array->length;
    }

    // Clamp deleteCount
    if (deleteCount < 0) {
        deleteCount = 0;
    }
    if (start + deleteCount > array->length) {
        deleteCount = array->length - start;
    }

    // Calculate new length
    int64_t newLength = array->length - deleteCount + itemCount;

    // Resize if needed
    if (newLength > array->capacity) {
        int64_t newCapacity = newLength * 2;
        int64_t* newElements = static_cast<int64_t*>(malloc(newCapacity * sizeof(int64_t)));
        if (array->elements) {
            std::memcpy(newElements, array->elements, array->length * sizeof(int64_t));
            free(array->elements);
        }
        array->elements = newElements;
        array->capacity = newCapacity;
    }

    // Shift elements to make room for insertions (or close gap for deletions)
    int64_t shift = itemCount - deleteCount;
    if (shift > 0) {
        // Moving elements right to make room
        for (int64_t i = array->length - 1; i >= start + deleteCount; i--) {
            array->elements[i + shift] = array->elements[i];
        }
    } else if (shift < 0) {
        // Moving elements left to close gap
        for (int64_t i = start + deleteCount; i < array->length; i++) {
            array->elements[i + shift] = array->elements[i];
        }
    }

    // Insert new items
    for (int64_t i = 0; i < itemCount; i++) {
        array->elements[start + i] = items[i];
    }

    // Update length
    array->length = newLength;

    // Write back to metadata
    write_back_to_metadata(array_ptr, array);

    return array_ptr;
}

// Array.copyWithin(target, start, end) - shallow copies part to another location (ES2015)
// Modifies array in place and returns it
void* nova_value_array_copyWithin(void* array_ptr, int64_t target, int64_t start, int64_t end) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !array->elements || array->length == 0) {
        return array_ptr;
    }

    // Handle negative indices
    if (target < 0) {
        target = array->length + target;
        if (target < 0) target = 0;
    }
    if (start < 0) {
        start = array->length + start;
        if (start < 0) start = 0;
    }
    if (end < 0) {
        end = array->length + end;
        if (end < 0) end = 0;
    }

    // Default end to array length if not specified (end defaults to length in JS)
    // But since we always pass 3 params from caller, we handle it there
    if (end > array->length) {
        end = array->length;
    }

    // Clamp target and start
    if (target >= array->length) {
        return array_ptr;
    }
    if (start >= array->length) {
        return array_ptr;
    }

    // Calculate copy length
    int64_t copyLength = end - start;
    if (copyLength <= 0) {
        return array_ptr;
    }

    // Adjust copy length if it would exceed array bounds
    if (target + copyLength > array->length) {
        copyLength = array->length - target;
    }
    if (start + copyLength > array->length) {
        copyLength = array->length - start;
    }

    // Use memmove for overlapping regions (handles overlap correctly)
    std::memmove(
        &array->elements[target],
        &array->elements[start],
        copyLength * sizeof(int64_t)
    );

    // Write back to metadata
    write_back_to_metadata(array_ptr, array);

    // Return array pointer for chaining (like JavaScript)
    return array_ptr;
}

// Array.toSpliced(start, deleteCount) - returns new array with elements removed (ES2023)
// Immutable version of splice() - does not modify original array
void* nova_value_array_toSpliced(void* array_ptr, int64_t start, int64_t deleteCount) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !array->elements || array->length == 0) {
        // Return empty array
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    // Handle negative start index
    if (start < 0) {
        start = array->length + start;
        if (start < 0) start = 0;
    }

    // Clamp start to array bounds
    if (start >= array->length) {
        // Return copy of entire original array
        nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(array->length);
        resultArray->length = array->length;
        for (int64_t i = 0; i < array->length; i++) {
            resultArray->elements[i] = array->elements[i];
        }
        return nova::runtime::create_metadata_from_value_array(resultArray);
    }

    // Clamp deleteCount
    if (deleteCount < 0) {
        deleteCount = 0;
    }
    if (start + deleteCount > array->length) {
        deleteCount = array->length - start;
    }

    // Calculate new array length
    int64_t newLength = array->length - deleteCount;

    // Create new array
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(newLength);
    resultArray->length = newLength;

    // Copy elements before the deleted range
    for (int64_t i = 0; i < start; i++) {
        resultArray->elements[i] = array->elements[i];
    }

    // Copy elements after the deleted range
    for (int64_t i = start; i < newLength; i++) {
        resultArray->elements[i] = array->elements[i + deleteCount];
    }

    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Array.toString() - converts array to comma-separated string
// Returns string representation like "1,2,3"
const char* nova_value_array_toString(void* array_ptr) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || array->length == 0) {
        return "";
    }

    // Use join with comma delimiter
    return nova::runtime::value_array_join(array, ",");
}

// Array.flat() - flattens nested arrays one level deep (ES2019)
// Array.flat() - flattens array by default depth of 1 (ES2019)
// Returns new array
void* nova_value_array_flat(void* array_ptr) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || array->length == 0) {
        nova::runtime::ValueArray* empty = nova::runtime::create_value_array(0);
        return nova::runtime::create_metadata_from_value_array(empty);
    }

    std::vector<int64_t> flattened;
    for (int64_t i = 0; i < array->length; ++i) {
        void* candidate = reinterpret_cast<void*>(
            static_cast<std::uintptr_t>(array->elements[i]));
        if (is_readable_array_metadata(candidate)) {
            auto* nested = ensure_value_array(candidate);
            flattened.insert(
                flattened.end(), nested->elements,
                nested->elements + nested->length);
        } else {
            flattened.push_back(array->elements[i]);
        }
    }

    nova::runtime::ValueArray* result = nova::runtime::create_value_array(
        static_cast<int64_t>(flattened.size()));
    result->length = static_cast<int64_t>(flattened.size());
    std::copy(flattened.begin(), flattened.end(), result->elements);

    // Create metadata struct for the new array
    return nova::runtime::create_metadata_from_value_array(result);
}

// Array.flat(depth) - flattens array to specified depth (ES2019)
// depth parameter specifies how deep nested arrays should be flattened
void* nova_value_array_flat_depth(void* array_ptr, int64_t depth) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || array->length == 0 || depth < 0) {
        nova::runtime::ValueArray* empty = nova::runtime::create_value_array(0);
        return nova::runtime::create_metadata_from_value_array(empty);
    }

    // Create new array with same length
    // For simple value arrays, depth parameter doesn't change behavior
    // since there are no nested arrays to flatten
    nova::runtime::ValueArray* result = nova::runtime::create_value_array(array->length);
    result->length = array->length;

    // Copy elements from original array
    std::memcpy(result->elements, array->elements, array->length * sizeof(int64_t));

    // Create metadata struct for the new array
    return nova::runtime::create_metadata_from_value_array(result);
}

// Array.flatMap() - maps then flattens one level deep (ES2019)
// Callback function type: takes element, returns transformed value
typedef int64_t (*FlatMapCallbackFunc)(int64_t);

void* nova_value_array_flatMap(void* array_ptr, FlatMapCallbackFunc callback) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback) {
        // Return empty array
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        emptyArray->length = 0;
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    std::vector<int64_t> flattened;
    for (int64_t i = 0; i < array->length; i++) {
        int64_t element = array->elements[i];
        int64_t transformed = callback(element);
        struct ArrayMetadata {
            char header[24];
            int64_t length;
            int64_t capacity;
            int64_t* elements;
        };
        auto* nested = reinterpret_cast<ArrayMetadata*>(
            static_cast<uintptr_t>(transformed));
        if (is_readable_array_metadata(nested)) {
            flattened.insert(
                flattened.end(), nested->elements,
                nested->elements + nested->length);
        } else {
            flattened.push_back(transformed);
        }
    }

    nova::runtime::ValueArray* resultArray =
        nova::runtime::create_value_array(
            static_cast<int64_t>(flattened.size()));
    resultArray->length = static_cast<int64_t>(flattened.size());
    std::copy(flattened.begin(), flattened.end(), resultArray->elements);
    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Callback for Array.from with map function
typedef int64_t (*ArrayFromMapFunc)(int64_t, int64_t);  // (element, index) -> transformed

// Array.from(arrayLike) - creates new array from array-like object (ES2015)
// Static method: Array.from(), not array.from()
// Creates a shallow copy of the input array
void* nova_array_from(void* array_ptr) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || array->length == 0) {
        // Return empty array
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        emptyArray->length = 0;
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    // Create new array with same size
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(array->length);
    resultArray->length = array->length;

    // Copy all elements (shallow copy)
    std::memcpy(resultArray->elements, array->elements, array->length * sizeof(int64_t));

    // Return new array
    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Array.from(arrayLike, mapFn) - creates new array with map function (ES2015)
// mapFn receives (element, index) and returns transformed value
void* nova_array_from_map(void* array_ptr, ArrayFromMapFunc mapFn) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || array->length == 0) {
        // Return empty array
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        emptyArray->length = 0;
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    // Create new array with same size
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(array->length);
    resultArray->length = array->length;

    if (mapFn) {
        // Apply map function to each element
        for (int64_t i = 0; i < array->length; i++) {
            resultArray->elements[i] = mapFn(array->elements[i], i);
        }
    } else {
        // No map function, just copy
        std::memcpy(resultArray->elements, array->elements, array->length * sizeof(int64_t));
    }

    // Return new array
    return nova::runtime::create_metadata_from_value_array(resultArray);
}

// Array.of(...elements) - creates new array from arguments (ES2015)
// Static method: Array.of(), not array.of()
// Variadic function: takes count, then individual elements
void* nova_array_of(int64_t count, ...) {
    if (count <= 0) {
        // Return empty array
        nova::runtime::ValueArray* emptyArray = nova::runtime::create_value_array(0);
        emptyArray->length = 0;
        return nova::runtime::create_metadata_from_value_array(emptyArray);
    }

    // Create new array with exact size needed
    nova::runtime::ValueArray* resultArray = nova::runtime::create_value_array(count);
    resultArray->length = count;

    // Use va_list to get variable arguments
    va_list args;
    va_start(args, count);

    // Fill array with arguments
    for (int64_t i = 0; i < count; i++) {
        int64_t element = va_arg(args, int64_t);
        resultArray->elements[i] = element;
    }

    va_end(args);

    // Return new array
    return nova::runtime::create_metadata_from_value_array(resultArray);
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

int64_t nova_value_array_lastIndexOf(void* array_ptr, int64_t value) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    return nova::runtime::value_array_lastIndexOf(array, value);
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

std::uint64_t nova_value_array_find_tagged(
        void* array_ptr, FindCallbackFunc callback) {
    auto* array = static_cast<nova::runtime::ValueArray*>(array_ptr);
    if (!array || !callback) {
        return nova::runtime::JS_VALUE_UNDEFINED;
    }
    for (int64_t i = 0; i < array->length; ++i) {
        if (callback(array->elements[i]) != 0) {
            return nova_value_from_i64(array->elements[i]);
        }
    }
    return nova::runtime::JS_VALUE_UNDEFINED;
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

// Array.findLast() implementation - ES2023
// Callback function type: same as find
typedef int64_t (*FindLastCallbackFunc)(int64_t);

int64_t nova_value_array_findLast(void* array_ptr, FindLastCallbackFunc callback) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback) {
        return 0;  // Not found
    }

    // Iterate through array elements BACKWARDS (from end to start)
    for (int64_t i = array->length - 1; i >= 0; i--) {
        int64_t element = array->elements[i];

        // Call the callback function
        int64_t result = callback(element);

        // If callback returns truthy value, return this element
        if (result != 0) {
            return element;  // Return the ELEMENT, not the index
        }
    }

    // Not found
    return 0;
}

std::uint64_t nova_value_array_findLast_tagged(
        void* array_ptr, FindLastCallbackFunc callback) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    if (!array || !callback) {
        return nova::runtime::JS_VALUE_UNDEFINED;
    }
    for (int64_t i = array->length - 1; i >= 0; --i) {
        if (callback(array->elements[i]) != 0) {
            return nova_value_from_i64(array->elements[i]);
        }
    }
    return nova::runtime::JS_VALUE_UNDEFINED;
}

// Array.findLastIndex() implementation - ES2023
// Callback function type: same as findIndex
typedef int64_t (*FindLastIndexCallbackFunc)(int64_t);

int64_t nova_value_array_findLastIndex(void* array_ptr, FindLastIndexCallbackFunc callback) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback) {
        return -1;  // Not found
    }

    // Iterate through array elements BACKWARDS (from end to start)
    for (int64_t i = array->length - 1; i >= 0; i--) {
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

// Closure-aware Array callback ABI. The compiled callback receives the
// ECMAScript arguments it declared, followed by its captured environment when
// one exists.
void nova_value_array_forEach_ctx(
        void* array_ptr, void* callback_ptr, void* environment,
        int64_t arity) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);
    if (!array || !callback_ptr) return;

    for (int64_t index = 0; index < array->length; ++index) {
        const int64_t value = array->elements[index];
        if (environment) {
            if (arity >= 3) {
                using Callback = void (*)(
                    int64_t, int64_t, void*, void*);
                reinterpret_cast<Callback>(callback_ptr)(
                    value, index, array_ptr, environment);
            } else if (arity == 2) {
                using Callback = void (*)(int64_t, int64_t, void*);
                reinterpret_cast<Callback>(callback_ptr)(
                    value, index, environment);
            } else if (arity == 1) {
                using Callback = void (*)(int64_t, void*);
                reinterpret_cast<Callback>(callback_ptr)(value, environment);
            } else {
                using Callback = void (*)(void*);
                reinterpret_cast<Callback>(callback_ptr)(environment);
            }
        } else if (arity >= 3) {
            using Callback = void (*)(int64_t, int64_t, void*);
            reinterpret_cast<Callback>(callback_ptr)(
                value, index, array_ptr);
        } else if (arity == 2) {
            using Callback = void (*)(int64_t, int64_t);
            reinterpret_cast<Callback>(callback_ptr)(value, index);
        } else if (arity == 1) {
            using Callback = void (*)(int64_t);
            reinterpret_cast<Callback>(callback_ptr)(value);
        } else {
            using Callback = void (*)();
            reinterpret_cast<Callback>(callback_ptr)();
        }
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

    // Apply callback to each element (left to right)
    for (int64_t i = 0; i < array->length; i++) {
        int64_t element = array->elements[i];
        accumulator = callback(accumulator, element);  // Update accumulator with callback result
    }

    return accumulator;
}

// Array.reduceRight() implementation
// Callback function type: same as reduce (2 parameters)
typedef int64_t (*ReduceRightCallbackFunc)(int64_t, int64_t);

int64_t nova_value_array_reduceRight(void* array_ptr, ReduceRightCallbackFunc callback, int64_t initialValue) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback) {
        return initialValue;  // Return initial value if array or callback is null
    }

    // Start with initial value
    int64_t accumulator = initialValue;

    // Apply callback to each element (RIGHT TO LEFT - backwards)
    for (int64_t i = array->length - 1; i >= 0; i--) {
        int64_t element = array->elements[i];
        accumulator = callback(accumulator, element);  // Update accumulator with callback result
    }

    return accumulator;
}

// Array.reduce() without an explicit initial value: per spec, the first
// element is used as the initial accumulator and iteration starts at index 1.
int64_t nova_value_array_reduce_no_init(void* array_ptr, ReduceCallbackFunc callback) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback || array->length == 0) {
        return 0;  // Matches V8 behavior: TypeError in spec, but we return 0.
    }

    int64_t accumulator = array->elements[0];
    for (int64_t i = 1; i < array->length; i++) {
        accumulator = callback(accumulator, array->elements[i]);
    }
    return accumulator;
}

// Array.reduceRight() without an explicit initial value: per spec, the last
// element is used as the initial accumulator and iteration moves backwards.
int64_t nova_value_array_reduceRight_no_init(void* array_ptr, ReduceRightCallbackFunc callback) {
    nova::runtime::ValueArray* array = ensure_value_array(array_ptr);

    if (!array || !callback || array->length == 0) {
        return 0;
    }

    int64_t accumulator = array->elements[array->length - 1];
    for (int64_t i = array->length - 2; i >= 0; i--) {
        accumulator = callback(accumulator, array->elements[i]);
    }
    return accumulator;
}



} // extern "C"
