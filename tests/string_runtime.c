#include <string.h>
#include <stdlib.h>
#include <stdint.h>

const char* nova_string_concat_cstr(const char* a, const char* b) {
    if (!a && !b) return "";
    if (!a) return b;
    if (!b) return a;

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

int64_t nova_string_charAt(const char* str, int64_t index) {
    if (!str) return 0;

    size_t len = strlen(str);
    if (index < 0 || (size_t)index >= len) {
        return 0;
    }

    return (int64_t)str[index];
}

int64_t nova_string_indexOf(const char* str, const char* search) {
    if (!str || !search) return -1;

    const char* found = strstr(str, search);
    if (!found) return -1;

    return (int64_t)(found - str);
}

const char* nova_string_substring(const char* str, int64_t start, int64_t end) {
    if (!str) return "";

    size_t len = strlen(str);

    if (start < 0) start = 0;
    if (end < 0) end = 0;
    if ((size_t)start > len) start = len;
    if ((size_t)end > len) end = len;
    if (start > end) {
        int64_t temp = start;
        start = end;
        end = temp;
    }

    int64_t substr_len = end - start;
    if (substr_len <= 0) return "";

    char* result = (char*)malloc(substr_len + 1);
    if (!result) return "";

    memcpy(result, str + start, substr_len);
    result[substr_len] = '\0';

    return result;
}
