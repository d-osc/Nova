#include "nova/runtime/Runtime.h"
#include <iostream>
#include <cmath>
#include <random>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <string>

namespace nova {
namespace runtime {

// Utility functions
void print_value(void* value, TypeId type_id) {
    if (!value) {
        std::cout << "null";
        return;
    }
    
    switch (type_id) {
        case TypeId::OBJECT:
            std::cout << "[Object]";
            break;
        case TypeId::ARRAY: {
            Array* array = static_cast<Array*>(value);
            std::cout << "[Array length=" << array->length << "]";
            break;
        }
        case TypeId::STRING: {
            String* str = static_cast<String*>(value);
            std::cout << "\"" << str->data << "\"";
            break;
        }
        case TypeId::FUNCTION:
            std::cout << "[Function]";
            break;
        case TypeId::CLOSURE:
            std::cout << "[Closure]";
            break;
        default:
            std::cout << "[Unknown type " << static_cast<uint32>(type_id) << "]";
            break;
    }
}

void panic(const char* message) {
    std::cerr << "PANIC: " << (message ? message : "Unknown error") << std::endl;
    std::exit(1);
}

void assert_impl(bool condition, const char* message) {
    if (!condition) {
        panic(message);
    }
}

// Math functions
float64 math_abs(float64 x) {
    return std::abs(x);
}

float64 math_sqrt(float64 x) {
    if (x < 0.0) return std::nan("");
    return std::sqrt(x);
}

// Integer square root using Newton's method
int64 nova_math_sqrt_i64(int64 x) {
    if (x < 0) return 0;  // Return 0 for negative numbers
    if (x == 0 || x == 1) return x;

    // Newton's method for integer square root
    int64 result = x;
    int64 prev = 0;

    while (result != prev) {
        prev = result;
        result = (result + x / result) / 2;
    }

    return result;
}

float64 math_pow(float64 base, float64 exp) {
    return std::pow(base, exp);
}

float64 math_sin(float64 x) {
    return std::sin(x);
}

float64 math_cos(float64 x) {
    return std::cos(x);
}

float64 math_tan(float64 x) {
    return std::tan(x);
}

float64 math_log(float64 x) {
    if (x <= 0.0) return std::nan("");
    return std::log(x);
}

float64 math_exp(float64 x) {
    return std::exp(x);
}

// Random functions
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<uint32> uint_dist;

void random_seed(uint32 seed) {
    gen.seed(seed);
}

uint32 random_next() {
    return uint_dist(gen);
}

float64 random_float() {
    std::uniform_real_distribution<float64> real_dist(0.0, 1.0);
    return real_dist(gen);
}

// Time functions
uint64 current_time_millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

void sleep_ms(uint32 milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// I/O functions
void print_string(const char* str) {
    if (str) {
        std::cout << str;
    }
}

void print_int(int64 value) {
    std::cout << value;
}

void print_float(float64 value) {
    std::cout << value;
}

void print_bool(bool value) {
    std::cout << (value ? "true" : "false");
}

char* read_line() {
    std::string line;
    if (std::getline(std::cin, line)) {
        // Allocate a copy that the runtime can manage
        size_t length = line.length();
        char* result = static_cast<char*>(allocate(length + 1, TypeId::OBJECT));
        std::memcpy(result, line.c_str(), length);
        result[length] = '\0';
        return result;
    }
    return nullptr;
}

} // namespace runtime
} // namespace nova

// Extern "C" wrapper for console functions
extern "C" {

// console.log() - outputs string message to stdout
void nova_console_log_string(const char* str) {
    if (str) {
        printf("%s\n", str);
    }
}

// console.log() - outputs number to stdout
void nova_console_log_number(int64_t value) {
    printf("%lld\n", (long long)value);
}

// console.error() - outputs error message to stderr
void nova_console_error_string(const char* str) {
    if (str) {
        fprintf(stderr, "%s\n", str);
    }
}

// console.error() - outputs number to stderr
void nova_console_error_number(int64_t value) {
    fprintf(stderr, "%lld\n", (long long)value);
}

// console.warn() - outputs warning message to stderr
void nova_console_warn_string(const char* str) {
    if (str) {
        fprintf(stderr, "%s\n", str);
    }
}

// console.warn() - outputs number to stderr
void nova_console_warn_number(int64_t value) {
    fprintf(stderr, "%lld\n", (long long)value);
}

// console.info() - outputs info message to stdout
void nova_console_info_string(const char* str) {
    if (str) {
        printf("%s\n", str);
    }
}

// console.info() - outputs number to stdout
void nova_console_info_number(int64_t value) {
    printf("%lld\n", (long long)value);
}

// console.debug() - outputs debug message to stdout
void nova_console_debug_string(const char* str) {
    if (str) {
        printf("%s\n", str);
    }
}

// console.debug() - outputs number to stdout
void nova_console_debug_number(int64_t value) {
    printf("%lld\n", (long long)value);
}

// console.clear() - clears the console
void nova_console_clear() {
    // Use ANSI escape code to clear screen and reset cursor
    // This works on most modern terminals (Linux, macOS, Windows 10+)
    printf("\033[2J\033[H");
    fflush(stdout);
}

// Timer storage for console.time() / console.timeEnd()
static std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> timers;

// console.time(label) - starts a timer with the given label
void nova_console_time_string(const char* label) {
    if (!label) label = "default";
    timers[label] = std::chrono::high_resolution_clock::now();
}

// console.timeEnd(label) - stops the timer and prints elapsed time
void nova_console_timeEnd_string(const char* label) {
    if (!label) label = "default";

    auto it = timers.find(label);
    if (it == timers.end()) {
        printf("%s: Timer does not exist\n", label);
        return;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - it->second);
    printf("%s: %.3fms\n", label, duration.count() / 1.0);

    // Remove the timer
    timers.erase(it);
}

// console.assert(condition, message) - prints error if condition is false
void nova_console_assert(int64_t condition, const char* message) {
    // If condition is falsy (0), print assertion error to stderr
    if (!condition) {
        if (message) {
            fprintf(stderr, "Assertion failed: %s\n", message);
        } else {
            fprintf(stderr, "Assertion failed\n");
        }
    }
}

// Counter storage for console.count() / console.countReset()
static std::unordered_map<std::string, int64_t> counters;

// console.count(label) - increments and prints counter for label
void nova_console_count_string(const char* label) {
    if (!label) label = "default";

    // Increment counter
    counters[label]++;

    // Print label and count
    printf("%s: %lld\n", label, (long long)counters[label]);
}

// console.countReset(label) - resets counter to zero
void nova_console_countReset_string(const char* label) {
    if (!label) label = "default";

    // Reset counter to 0
    counters[label] = 0;
}

// console.table(data) - displays array data in tabular format
void nova_console_table_array(void* array_ptr) {
    // Cast to ValueArray (arrays in Nova are ValueArray for primitives)
    auto* array = reinterpret_cast<nova::runtime::ValueArray*>(array_ptr);

    if (!array || array->length == 0) {
        printf("(empty)\n");
        return;
    }

    // Print table header
    printf("┌─────────┬─────────────────────┐\n");
    printf("│ (index) │       Values        │\n");
    printf("├─────────┼─────────────────────┤\n");

    // Print each row
    for (int64_t i = 0; i < array->length; i++) {
        printf("│   %3lld   │ ", (long long)i);

        // Display value (ValueArray stores int64 values)
        printf("%19lld", (long long)array->elements[i]);

        printf(" │\n");
    }

    // Print table footer
    printf("└─────────┴─────────────────────┘\n");
}

// Group indentation tracking for console.group() / console.groupEnd()
static int group_indent_level = 0;

// Helper to print current indentation
static void print_indent() {
    for (int i = 0; i < group_indent_level; i++) {
        printf("  ");
    }
}

// console.group(label) - starts a new indented group with label
void nova_console_group_string(const char* label) {
    print_indent();
    if (label) {
        printf("▼ %s\n", label);
    } else {
        printf("▼ Group\n");
    }
    group_indent_level++;
}

// console.group() - starts a new indented group without label
void nova_console_group_default() {
    print_indent();
    printf("▼ Group\n");
    group_indent_level++;
}

// console.groupEnd() - ends the current group (decreases indentation)
void nova_console_groupEnd() {
    if (group_indent_level > 0) {
        group_indent_level--;
    }
}

// console.trace(message) - prints stack trace with message
// Note: Simplified implementation without full call stack
void nova_console_trace_string(const char* message) {
    if (message) {
        printf("Trace: %s\n", message);
    } else {
        printf("Trace\n");
    }
    // In a full implementation, this would print the call stack
    // For now, we just print the message with a "Trace:" prefix
}

// console.trace() - prints stack trace without message
void nova_console_trace_default() {
    printf("Trace\n");
    // In a full implementation, this would print the call stack
}

// console.dir(value) - displays value properties in readable format
// For numbers
void nova_console_dir_number(int64_t value) {
    printf("Number: %lld\n", (long long)value);
}

// For strings
void nova_console_dir_string(const char* str) {
    if (str) {
        printf("String: \"%s\" (length: %zu)\n", str, strlen(str));
    } else {
        printf("String: null\n");
    }
}

// For arrays
void nova_console_dir_array(void* array_ptr) {
    auto* array = reinterpret_cast<nova::runtime::ValueArray*>(array_ptr);

    if (!array) {
        printf("Array: null\n");
        return;
    }

    printf("Array: [");
    for (int64_t i = 0; i < array->length; i++) {
        if (i > 0) printf(", ");
        printf("%lld", (long long)array->elements[i]);
    }
    printf("] (length: %lld)\n", (long long)array->length);
}

// Date.now() - returns current timestamp in milliseconds since Unix epoch (ES5)
int64_t nova_date_now() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return static_cast<int64_t>(millis.count());
}

// Static variable to store the start time for performance.now()
static auto performance_start_time = std::chrono::high_resolution_clock::now();

// performance.now() - returns high-resolution timestamp in milliseconds (Web Performance API)
// Returns time since process/page start with sub-millisecond precision
double nova_performance_now() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now - performance_start_time;
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(duration);
    // Return milliseconds with microsecond precision
    return static_cast<double>(micros.count()) / 1000.0;
}

// Math.min(a, b) - returns the smaller of two values (ES1)
int64_t nova_math_min(int64_t a, int64_t b) {
    return a < b ? a : b;
}

// Math.max(a, b) - returns the larger of two values (ES1)
int64_t nova_math_max(int64_t a, int64_t b) {
    return a > b ? a : b;
}

// JSON.stringify(number) - converts a number to a JSON string (ES5)
char* nova_json_stringify_number(int64_t value) {
    // Convert number to string
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%lld", (long long)value);

    // Allocate and copy result
    size_t len = strlen(buffer);
    char* result = (char*)malloc(len + 1);
    strcpy(result, buffer);
    return result;
}

// JSON.stringify(string) - converts a string to a JSON string with quotes (ES5)
char* nova_json_stringify_string(const char* str) {
    if (!str) {
        // Return "null" for null strings
        char* result = (char*)malloc(5);
        strcpy(result, "null");
        return result;
    }

    // Calculate length: original + 2 quotes + null terminator
    size_t len = strlen(str);
    char* result = (char*)malloc(len + 3);

    // Add opening quote, string, and closing quote
    result[0] = '"';
    strcpy(result + 1, str);
    result[len + 1] = '"';
    result[len + 2] = '\0';

    return result;
}

// JSON.stringify(boolean) - converts a boolean to a JSON string (ES5)
char* nova_json_stringify_bool(int64_t value) {
    if (value) {
        char* result = (char*)malloc(5);
        strcpy(result, "true");
        return result;
    } else {
        char* result = (char*)malloc(6);
        strcpy(result, "false");
        return result;
    }
}

// encodeURIComponent() - encodes a URI component (ES3)
// Encodes all characters except: A-Z a-z 0-9 - _ . ! ~ * ' ( )
char* nova_encodeURIComponent(const char* str) {
    if (!str) {
        char* result = (char*)malloc(1);
        result[0] = '\0';
        return result;
    }

    // First pass: calculate result length
    size_t len = strlen(str);
    size_t resultLen = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        // Characters that don't need encoding
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '!' ||
            c == '~' || c == '*' || c == '\'' || c == '(' || c == ')') {
            resultLen += 1;
        } else {
            resultLen += 3;  // %XX format
        }
    }

    // Allocate result buffer
    char* result = (char*)malloc(resultLen + 1);
    char* ptr = result;

    // Second pass: encode
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        // Characters that don't need encoding
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '!' ||
            c == '~' || c == '*' || c == '\'' || c == '(' || c == ')') {
            *ptr++ = c;
        } else {
            // Encode as %XX
            sprintf(ptr, "%%%02X", c);
            ptr += 3;
        }
    }
    *ptr = '\0';

    return result;
}

// Helper function to convert hex character to value
static int hexCharToValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;  // Invalid hex character
}

// decodeURIComponent() - decodes a URI component (ES3)
// Decodes percent-encoded characters back to original
char* nova_decodeURIComponent(const char* str) {
    if (!str) {
        char* result = (char*)malloc(1);
        result[0] = '\0';
        return result;
    }

    size_t len = strlen(str);
    // Result will be at most the same length as input
    char* result = (char*)malloc(len + 1);
    char* ptr = result;

    for (size_t i = 0; i < len; i++) {
        if (str[i] == '%' && i + 2 < len) {
            int high = hexCharToValue(str[i + 1]);
            int low = hexCharToValue(str[i + 2]);
            if (high >= 0 && low >= 0) {
                *ptr++ = (char)((high << 4) | low);
                i += 2;  // Skip the two hex digits
            } else {
                // Invalid escape sequence, copy as-is
                *ptr++ = str[i];
            }
        } else {
            *ptr++ = str[i];
        }
    }
    *ptr = '\0';

    return result;
}

// Base64 encoding table
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// btoa() - encodes a string to base64 (Web API)
char* nova_btoa(const char* str) {
    if (!str) {
        char* result = (char*)malloc(1);
        result[0] = '\0';
        return result;
    }

    size_t len = strlen(str);
    // Calculate output length: ceil(len / 3) * 4
    size_t outLen = ((len + 2) / 3) * 4;
    char* result = (char*)malloc(outLen + 1);
    char* ptr = result;

    for (size_t i = 0; i < len; i += 3) {
        // Get up to 3 bytes
        unsigned char b0 = (unsigned char)str[i];
        unsigned char b1 = (i + 1 < len) ? (unsigned char)str[i + 1] : 0;
        unsigned char b2 = (i + 2 < len) ? (unsigned char)str[i + 2] : 0;

        // How many bytes we actually have in this group
        int remaining = len - i;

        // Encode to 4 base64 characters
        *ptr++ = base64_chars[b0 >> 2];
        *ptr++ = base64_chars[((b0 & 0x03) << 4) | (b1 >> 4)];

        if (remaining >= 2) {
            *ptr++ = base64_chars[((b1 & 0x0F) << 2) | (b2 >> 6)];
        } else {
            *ptr++ = '=';
        }

        if (remaining >= 3) {
            *ptr++ = base64_chars[b2 & 0x3F];
        } else {
            *ptr++ = '=';
        }
    }
    *ptr = '\0';

    return result;
}

} // extern "C"