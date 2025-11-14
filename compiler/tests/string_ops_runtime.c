#include <stdlib.h>
#include <string.h>

// Runtime function for string concatenation
const char* nova_string_concat_cstr(const char* a, const char* b) {
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    size_t total_len = len_a + len_b;

    char* result = (char*)malloc(total_len + 1);
    if (!result) return "";

    memcpy(result, a, len_a);
    memcpy(result + len_a, b, len_b);
    result[total_len] = '\0';

    return result;
}
