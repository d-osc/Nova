#include "nova/runtime/Runtime.h"
#include <cstring>
#include <algorithm>

namespace nova {
namespace runtime {

String* create_string(const char* data) {
    if (!data) return create_string("", 0);
    
    size_t length = std::strlen(data);
    return create_string(data, static_cast<int64>(length));
}

String* create_string(const char* data, int64 length) {
    if (!data) return create_string("", 0);
    if (length < 0) length = 0;
    
    // Allocate string structure
    String* str = static_cast<String*>(allocate(sizeof(String), TypeId::STRING));
    
    // Initialize string
    str->length = length;
    
    // Allocate data
    size_t data_size = (length + 1) * sizeof(char);
    str->data = static_cast<char*>(allocate(data_size, TypeId::OBJECT));
    
    // Copy data
    std::memcpy(str->data, data, length);
    str->data[length] = '\0'; // Null-terminate
    
    return str;
}

String* create_string(const std::string& str) {
    return create_string(str.c_str(), static_cast<int64>(str.length()));
}

const char* string_data(String* str) {
    return str ? str->data : "";
}

int64 string_length(String* str) {
    return str ? str->length : 0;
}

String* string_concat(String* a, String* b) {
    if (!a && !b) return create_string("", 0);
    if (!a) return create_string(string_data(b), string_length(b));
    if (!b) return create_string(string_data(a), string_length(a));
    
    int64 new_length = a->length + b->length;
    String* result = static_cast<String*>(allocate(sizeof(String), TypeId::STRING));
    
    // Initialize result
    result->length = new_length;
    
    // Allocate data
    size_t data_size = (new_length + 1) * sizeof(char);
    result->data = static_cast<char*>(allocate(data_size, TypeId::OBJECT));
    
    // Copy data
    std::memcpy(result->data, a->data, a->length);
    std::memcpy(result->data + a->length, b->data, b->length);
    result->data[new_length] = '\0'; // Null-terminate
    
    return result;
}

int32 string_compare(String* a, String* b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    
    // Compare lengths first
    if (a->length < b->length) return -1;
    if (a->length > b->length) return 1;
    
    // Compare contents
    return std::memcmp(a->data, b->data, a->length);
}

} // namespace runtime
} // namespace nova

// ==================== C-Style String Functions (for AOT compatibility) ====================

extern "C" {

// Simple C-string concatenation for LLVM code generation
const char* nova_string_concat_cstr(const char* a, const char* b) {
    if (!a && !b) return "";
    if (!a) return b;
    if (!b) return a;

    size_t len_a = std::strlen(a);
    size_t len_b = std::strlen(b);
    size_t total_len = len_a + len_b;

    // Allocate memory for result (using malloc for simplicity)
    char* result = static_cast<char*>(malloc(total_len + 1));
    if (!result) return "";

    // Copy strings
    std::memcpy(result, a, len_a);
    std::memcpy(result + len_a, b, len_b);
    result[total_len] = '\0';

    return result;
}

// Get character at index (returns 0 if out of bounds)
int64_t nova_string_charAt(const char* str, int64_t index) {
    if (!str) return 0;

    size_t len = std::strlen(str);
    if (index < 0 || static_cast<size_t>(index) >= len) {
        return 0; // Out of bounds
    }

    return static_cast<int64_t>(str[index]);
}

// Find first occurrence of substring, returns -1 if not found
int64_t nova_string_indexOf(const char* str, const char* search) {
    if (!str || !search) return -1;

    const char* found = std::strstr(str, search);
    if (!found) return -1;

    return static_cast<int64_t>(found - str);
}

// Extract substring from start to end (exclusive)
const char* nova_string_substring(const char* str, int64_t start, int64_t end) {
    if (!str) return "";

    size_t len = std::strlen(str);

    // Clamp start and end to valid range
    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if (static_cast<size_t>(start) > len) start = len;
    if (static_cast<size_t>(end) > len) end = len;
    if (start > end) {
        // Swap if needed
        int64_t temp = start;
        start = end;
        end = temp;
    }

    int64_t substr_len = end - start;
    if (substr_len <= 0) return "";

    // Allocate result
    char* result = static_cast<char*>(malloc(substr_len + 1));
    if (!result) return "";

    // Copy substring
    std::memcpy(result, str + start, substr_len);
    result[substr_len] = '\0';

    return result;
}

// Convert string to lowercase
const char* nova_string_toLowerCase(const char* str) {
    if (!str) return "";

    size_t len = std::strlen(str);
    char* result = static_cast<char*>(malloc(len + 1));
    if (!result) return "";

    for (size_t i = 0; i < len; i++) {
        result[i] = std::tolower(static_cast<unsigned char>(str[i]));
    }
    result[len] = '\0';

    return result;
}

// Convert string to uppercase
const char* nova_string_toUpperCase(const char* str) {
    if (!str) return "";

    size_t len = std::strlen(str);
    char* result = static_cast<char*>(malloc(len + 1));
    if (!result) return "";

    for (size_t i = 0; i < len; i++) {
        result[i] = std::toupper(static_cast<unsigned char>(str[i]));
    }
    result[len] = '\0';

    return result;
}

// Trim whitespace from both ends of string
const char* nova_string_trim(const char* str) {
    if (!str) return "";

    size_t len = std::strlen(str);
    if (len == 0) return "";

    // Find first non-whitespace character
    size_t start = 0;
    while (start < len && std::isspace(static_cast<unsigned char>(str[start]))) {
        start++;
    }

    // If all whitespace, return empty string
    if (start == len) return "";

    // Find last non-whitespace character
    size_t end = len - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(str[end]))) {
        end--;
    }

    // Calculate trimmed length
    size_t trimmed_len = end - start + 1;

    // Allocate result
    char* result = static_cast<char*>(malloc(trimmed_len + 1));
    if (!result) return "";

    // Copy trimmed string
    std::memcpy(result, str + start, trimmed_len);
    result[trimmed_len] = '\0';

    return result;
}

// Check if string starts with a prefix
int64_t nova_string_startsWith(const char* str, const char* prefix) {
    if (!str || !prefix) return 0;

    size_t str_len = std::strlen(str);
    size_t prefix_len = std::strlen(prefix);

    // If prefix is longer than string, it can't match
    if (prefix_len > str_len) return 0;

    // Compare prefix
    for (size_t i = 0; i < prefix_len; i++) {
        if (str[i] != prefix[i]) return 0;
    }

    return 1;  // Match found
}

// Check if string ends with a suffix
int64_t nova_string_endsWith(const char* str, const char* suffix) {
    if (!str || !suffix) return 0;

    size_t str_len = std::strlen(str);
    size_t suffix_len = std::strlen(suffix);

    // If suffix is longer than string, it can't match
    if (suffix_len > str_len) return 0;

    // Compare suffix (starting from the end)
    size_t offset = str_len - suffix_len;
    for (size_t i = 0; i < suffix_len; i++) {
        if (str[offset + i] != suffix[i]) return 0;
    }

    return 1;  // Match found
}

// Repeat string n times
const char* nova_string_repeat(const char* str, int64_t count) {
    if (!str || count <= 0) return "";

    size_t str_len = std::strlen(str);
    if (str_len == 0) return "";

    // Calculate total length
    size_t total_len = str_len * count;

    // Allocate result
    char* result = static_cast<char*>(malloc(total_len + 1));
    if (!result) return "";

    // Repeat the string
    for (int64_t i = 0; i < count; i++) {
        std::memcpy(result + (i * str_len), str, str_len);
    }
    result[total_len] = '\0';

    return result;
}

// Check if string contains a substring
int64_t nova_string_includes(const char* str, const char* search) {
    if (!str || !search) return 0;

    // Use strstr to find the substring
    const char* found = std::strstr(str, search);
    return found ? 1 : 0;
}

// Extract slice of string (similar to substring, supports negative indices)
const char* nova_string_slice(const char* str, int64_t start, int64_t end) {
    if (!str) return "";

    int64_t len = static_cast<int64_t>(std::strlen(str));

    // Handle negative indices
    if (start < 0) start = std::max(len + start, static_cast<int64_t>(0));
    if (end < 0) end = std::max(len + end, static_cast<int64_t>(0));

    // Clamp to valid range
    if (start > len) start = len;
    if (end > len) end = len;
    if (start > end) start = end;  // If start > end, return empty string

    int64_t slice_len = end - start;
    if (slice_len <= 0) return "";

    // Allocate result
    char* result = static_cast<char*>(malloc(slice_len + 1));
    if (!result) return "";

    // Copy slice
    std::memcpy(result, str + start, slice_len);
    result[slice_len] = '\0';

    return result;
}

// Replace first occurrence of search string with replacement
const char* nova_string_replace(const char* str, const char* search, const char* replace) {
    if (!str) return "";
    if (!search || !replace) return str;

    size_t str_len = std::strlen(str);
    size_t search_len = std::strlen(search);
    size_t replace_len = std::strlen(replace);

    // If search is empty, return original string
    if (search_len == 0) return str;

    // Find first occurrence
    const char* found = std::strstr(str, search);
    if (!found) return str;  // Not found, return original

    // Calculate position and new length
    size_t pos = found - str;
    size_t new_len = str_len - search_len + replace_len;

    // Allocate result
    char* result = static_cast<char*>(malloc(new_len + 1));
    if (!result) return str;

    // Copy parts: before + replacement + after
    std::memcpy(result, str, pos);
    std::memcpy(result + pos, replace, replace_len);
    std::memcpy(result + pos + replace_len, str + pos + search_len, str_len - pos - search_len);
    result[new_len] = '\0';

    return result;
}

// Pad string to target length by prepending fill string
const char* nova_string_padStart(const char* str, int64_t target_len, const char* fill) {
    if (!str) return "";
    if (!fill || target_len <= 0) return str;

    int64_t str_len = static_cast<int64_t>(std::strlen(str));
    size_t fill_len = std::strlen(fill);

    // If already at or exceeds target length, return original
    if (str_len >= target_len) return str;
    if (fill_len == 0) return str;

    int64_t pad_len = target_len - str_len;

    // Allocate result
    char* result = static_cast<char*>(malloc(target_len + 1));
    if (!result) return str;

    // Fill with repeated fill string
    int64_t written = 0;
    while (written < pad_len) {
        int64_t to_copy = std::min(static_cast<int64_t>(fill_len), pad_len - written);
        std::memcpy(result + written, fill, to_copy);
        written += to_copy;
    }

    // Copy original string
    std::memcpy(result + pad_len, str, str_len);
    result[target_len] = '\0';

    return result;
}

// Pad string to target length by appending fill string
const char* nova_string_padEnd(const char* str, int64_t target_len, const char* fill) {
    if (!str) return "";
    if (!fill || target_len <= 0) return str;

    int64_t str_len = static_cast<int64_t>(std::strlen(str));
    size_t fill_len = std::strlen(fill);

    // If already at or exceeds target length, return original
    if (str_len >= target_len) return str;
    if (fill_len == 0) return str;

    int64_t pad_len = target_len - str_len;

    // Allocate result
    char* result = static_cast<char*>(malloc(target_len + 1));
    if (!result) return str;

    // Copy original string first
    std::memcpy(result, str, str_len);

    // Fill rest with repeated fill string
    int64_t written = 0;
    while (written < pad_len) {
        int64_t to_copy = std::min(static_cast<int64_t>(fill_len), pad_len - written);
        std::memcpy(result + str_len + written, fill, to_copy);
        written += to_copy;
    }

    result[target_len] = '\0';

    return result;
}


// Create a new string array with initial capacity
nova::runtime::StringArray* nova_string_array_create(int64_t capacity) {
    using namespace nova::runtime;

    if (capacity < 0) capacity = 0;

    // Allocate StringArray structure
    StringArray* array = static_cast<StringArray*>(allocate(sizeof(StringArray), TypeId::ARRAY));

    // Initialize array
    array->length = 0;
    array->capacity = capacity;

    // Allocate elements array
    if (capacity > 0) {
        size_t elements_size = capacity * sizeof(const char*);
        array->elements = static_cast<const char**>(allocate(elements_size, TypeId::OBJECT));
        // Initialize to null
        for (int64_t i = 0; i < capacity; i++) {
            array->elements[i] = nullptr;
        }
    } else {
        array->elements = nullptr;
    }

    return array;
}

// Split string by delimiter
void* nova_string_split(const char* str, const char* delimiter) {
    if (!str) return nova_string_array_create(0);
    if (!delimiter) return nova_string_array_create(1);

    size_t str_len = std::strlen(str);
    size_t delim_len = std::strlen(delimiter);

    if (delim_len == 0) {
        auto* array = nova_string_array_create(1);
        array->length = 1;
        array->elements[0] = str;
        return array;
    }

    int64_t count = 1;
    const char* pos = str;
    while ((pos = std::strstr(pos, delimiter)) != nullptr) {
        count++;
        pos += delim_len;
    }

    auto* array = nova_string_array_create(count);
    array->length = count;

    int64_t index = 0;
    const char* start = str;
    pos = str;

    while ((pos = std::strstr(pos, delimiter)) != nullptr) {
        int64_t part_len = pos - start;
        char* part = static_cast<char*>(malloc(part_len + 1));
        if (part) {
            std::memcpy(part, start, part_len);
            part[part_len] = 0;
            array->elements[index++] = part;
        }
        pos += delim_len;
        start = pos;
    }

    int64_t last_len = (str + str_len) - start;
    char* last_part = static_cast<char*>(malloc(last_len + 1));
    if (last_part) {
        std::memcpy(last_part, start, last_len);
        last_part[last_len] = 0;
        array->elements[index] = last_part;
    }

    return array;
}

}
