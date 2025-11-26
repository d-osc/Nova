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

}