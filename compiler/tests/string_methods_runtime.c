#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Runtime function for string.substring(start, end)
const char* nova_string_substring(const char* str, int64_t start, int64_t end) {
    if (!str) return "";

    size_t len = strlen(str);

    // Handle negative indices and bounds
    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if (start > (int64_t)len) start = len;
    if (end > (int64_t)len) end = len;
    if (start > end) start = end;

    size_t result_len = end - start;
    char* result = (char*)malloc(result_len + 1);
    if (!result) return "";

    memcpy(result, str + start, result_len);
    result[result_len] = '\0';

    return result;
}

// Runtime function for string.indexOf(searchStr)
int64_t nova_string_indexOf(const char* str, const char* searchStr) {
    if (!str || !searchStr) return -1;

    const char* pos = strstr(str, searchStr);
    if (!pos) return -1;

    return (int64_t)(pos - str);
}

// Runtime function for string.charAt(index)
const char* nova_string_charAt(const char* str, int64_t index) {
    if (!str) return "";

    size_t len = strlen(str);
    if (index < 0 || index >= (int64_t)len) return "";

    char* result = (char*)malloc(2);
    if (!result) return "";

    result[0] = str[index];
    result[1] = '\0';

    return result;
}

// Runtime function for string concatenation (already exists but included for completeness)
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
