#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple stub for nova_string_concat_cstr
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

// Main test
int main() {
    const char* s1 = "Hello";
    const char* s2 = " World";
    const char* result = nova_string_concat_cstr(s1, s2);
    
    printf("Concatenated: %s\n", result);
    printf("Expected: Hello World\n");
    
    // Compare
    if (strcmp(result, "Hello World") == 0) {
        printf("SUCCESS!\n");
        return 0;
    } else {
        printf("FAILED!\n");
        return 1;
    }
}
