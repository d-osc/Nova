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