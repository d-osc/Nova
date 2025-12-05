#include "nova/runtime/Runtime.h"
#include <iostream>
#include <cmath>
#include <random>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <unordered_map>
#include <string>
#include <csetjmp>

// Global exception handling state
static thread_local bool g_exception_pending = false;
static thread_local int64_t g_exception_value = 0;
[[maybe_unused]] static thread_local jmp_buf* g_exception_handler = nullptr;
[[maybe_unused]] static thread_local jmp_buf g_exception_buffer;  // The actual buffer
static thread_local int g_try_depth = 0;  // Track try nesting

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

[[noreturn]] void panic(const char* message) {
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

// console.timeLog(label) - prints elapsed time WITHOUT removing timer
void nova_console_timeLog_string(const char* label) {
    if (!label) label = "default";

    auto it = timers.find(label);
    if (it == timers.end()) {
        printf("%s: Timer does not exist\n", label);
        return;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - it->second);
    printf("%s: %.3fms\n", label, duration.count() / 1.0);
    // Note: Timer continues running (not erased)
}

// console.timeStamp(label) - adds timestamp marker (browser dev tools feature)
void nova_console_timeStamp_string(const char* label) {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    if (label) {
        printf("[%lld] %s\n", (long long)ms, label);
    } else {
        printf("[%lld]\n", (long long)ms);
    }
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

// console.groupCollapsed(label) - starts a collapsed group (same as group in CLI)
void nova_console_groupCollapsed_string(const char* label) {
    print_indent();
    if (label) {
        printf("▶ %s\n", label);  // Use ▶ to indicate collapsed
    } else {
        printf("▶ Group\n");
    }
    group_indent_level++;
}

// console.groupCollapsed() - starts a collapsed group without label
void nova_console_groupCollapsed_default() {
    print_indent();
    printf("▶ Group\n");
    group_indent_level++;
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

// For objects (simple key-value display)
void nova_console_dir_object(void* obj_ptr, int depth) {
    if (!obj_ptr) {
        printf("Object: null\n");
        return;
    }
    // Simplified - just show object exists
    printf("Object { ... } (depth: %d)\n", depth);
}

// console.dirxml(value) - displays as XML (for HTML/XML elements, falls back to dir)
void nova_console_dirxml_string(const char* str) {
    if (str) {
        printf("%s\n", str);
    } else {
        printf("null\n");
    }
}

void nova_console_dirxml_object(void* obj_ptr) {
    // In CLI environment, behaves same as dir
    nova_console_dir_object(obj_ptr, 2);
}

// ============================================================================
// console.profile() / console.profileEnd() - CPU profiling
// ============================================================================

static std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> profiles;

// console.profile(label) - starts CPU profiler
void nova_console_profile_string(const char* label) {
    if (!label) label = "default";
    profiles[label] = std::chrono::high_resolution_clock::now();
    printf("Profile '%s' started\n", label);
}

void nova_console_profile_default() {
    nova_console_profile_string("default");
}

// console.profileEnd(label) - stops CPU profiler
void nova_console_profileEnd_string(const char* label) {
    if (!label) label = "default";

    auto it = profiles.find(label);
    if (it == profiles.end()) {
        printf("Profile '%s' does not exist\n", label);
        return;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - it->second);
    printf("Profile '%s' finished: %.3fms\n", label, duration.count() / 1000.0);
    profiles.erase(it);
}

void nova_console_profileEnd_default() {
    nova_console_profileEnd_string("default");
}

// ============================================================================
// Additional console overloads for various types
// ============================================================================

// console.log for double/float
void nova_console_log_double(double value) {
    printf("%g\n", value);
}

// console.log for bool
void nova_console_log_bool(int value) {
    printf("%s\n", value ? "true" : "false");
}

// console.log for object
void nova_console_log_object(void* obj) {
    if (obj) {
        printf("[object Object]\n");
    } else {
        printf("null\n");
    }
}

// console.error for double
void nova_console_error_double(double value) {
    fprintf(stderr, "%g\n", value);
}

// console.error for bool
void nova_console_error_bool(int value) {
    fprintf(stderr, "%s\n", value ? "true" : "false");
}

// console.warn for double
void nova_console_warn_double(double value) {
    fprintf(stderr, "%g\n", value);
}

// console.warn for bool
void nova_console_warn_bool(int value) {
    fprintf(stderr, "%s\n", value ? "true" : "false");
}

// console.info for double
void nova_console_info_double(double value) {
    printf("%g\n", value);
}

// console.info for bool
void nova_console_info_bool(int value) {
    printf("%s\n", value ? "true" : "false");
}

// console.debug for double
void nova_console_debug_double(double value) {
    printf("%g\n", value);
}

// console.debug for bool
void nova_console_debug_bool(int value) {
    printf("%s\n", value ? "true" : "false");
}

// ============================================================================
// Console class constructor support
// ============================================================================

struct ConsoleInstance {
    FILE* stdout_stream;
    FILE* stderr_stream;
    int colorMode;   // 0=auto, 1=force, 2=disable
    int inspectOptions_depth;
    int groupIndentation;
};

// Create new Console instance
void* nova_console_create(void* stdout_stream, void* stderr_stream) {
    ConsoleInstance* console = new ConsoleInstance();
    console->stdout_stream = stdout_stream ? (FILE*)stdout_stream : stdout;
    console->stderr_stream = stderr_stream ? (FILE*)stderr_stream : stderr;
    console->colorMode = 0;
    console->inspectOptions_depth = 2;
    console->groupIndentation = 2;
    return console;
}

// Free Console instance
void nova_console_free(void* console_ptr) {
    if (console_ptr) {
        delete (ConsoleInstance*)console_ptr;
    }
}

// Console instance methods
void nova_console_instance_log(void* console_ptr, const char* str) {
    if (!console_ptr || !str) return;
    ConsoleInstance* console = (ConsoleInstance*)console_ptr;
    fprintf(console->stdout_stream, "%s\n", str);
}

void nova_console_instance_error(void* console_ptr, const char* str) {
    if (!console_ptr || !str) return;
    ConsoleInstance* console = (ConsoleInstance*)console_ptr;
    fprintf(console->stderr_stream, "%s\n", str);
}

void nova_console_instance_warn(void* console_ptr, const char* str) {
    if (!console_ptr || !str) return;
    ConsoleInstance* console = (ConsoleInstance*)console_ptr;
    fprintf(console->stderr_stream, "%s\n", str);
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

// JSON functions - wrap in extern "C" for proper linkage
extern "C" {

// String pool for constant strings (avoid malloc overhead)
static const char* const JSON_TRUE = "true";
static const char* const JSON_FALSE = "false";
static const char* const JSON_NULL = "null";

// Optimized integer to string (faster than snprintf)
static inline size_t fast_i64toa(int64_t value, char* buffer) {
    char* ptr = buffer;

    // Handle negative numbers
    if (value < 0) {
        *ptr++ = '-';
        value = -value;
    }

    // Handle zero case
    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return ptr - buffer;
    }

    // Convert digits in reverse
    char* start = ptr;
    while (value > 0) {
        *ptr++ = '0' + (value % 10);
        value /= 10;
    }

    // Reverse the digits
    char* end = ptr - 1;
    while (start < end) {
        char tmp = *start;
        *start++ = *end;
        *end-- = tmp;
    }

    *ptr = '\0';
    return ptr - buffer;
}

// JSON.stringify(number) - ULTRA OPTIMIZED - converts a number to a JSON string (ES5)
char* nova_json_stringify_number(int64_t value) {
    // For small numbers (0-9), use cached single-digit strings
    if (value >= 0 && value <= 9) {
        static const char* digits[10] = {"0","1","2","3","4","5","6","7","8","9"};
        return (char*)digits[value];
    }

    char buffer[32];
    size_t len = fast_i64toa(value, buffer);

    char* result = (char*)malloc(len + 1);
    memcpy(result, buffer, len + 1);
    return result;
}

// JSON.stringify(string) - ULTRA OPTIMIZED - converts a string to a JSON string with quotes (ES5)
char* nova_json_stringify_string(const char* str) {
    if (!str) {
        return (char*)JSON_NULL;
    }

    size_t len = strlen(str);

    // For empty strings, return cached result
    if (len == 0) {
        static const char* EMPTY_STRING = "\"\"";
        return (char*)EMPTY_STRING;
    }

    char* result = (char*)malloc(len + 3);

    // Unrolled copy for small strings
    result[0] = '"';
    if (len <= 16) {
        // Manual unrolling for cache efficiency
        for (size_t i = 0; i < len; i++) {
            result[i + 1] = str[i];
        }
    } else {
        memcpy(result + 1, str, len);
    }
    result[len + 1] = '"';
    result[len + 2] = '\0';

    return result;
}

// JSON.stringify(boolean) - ULTRA OPTIMIZED - returns constant strings (ES5)
char* nova_json_stringify_bool(int64_t value) {
    return (char*)(value ? JSON_TRUE : JSON_FALSE);
}

// JSON.stringify(null) - OPTIMIZED - returns constant string (ES5)
char* nova_json_stringify_null() {
    return (char*)JSON_NULL;
}

// JSON.stringify(undefined) - OPTIMIZED - returns constant string (ES5)
char* nova_json_stringify_undefined() {
    static const char* JSON_UNDEFINED = "undefined";
    return (char*)JSON_UNDEFINED;
}

// JSON.stringify(float) - converts a floating point number to a JSON string (ES5)
char* nova_json_stringify_float(double value) {
    char buffer[64];

    // Handle special values
    if (std::isnan(value)) {  // NaN check
        char* result = (char*)malloc(5);
        strcpy(result, "null");  // NaN becomes null in JSON
        return result;
    }
    if (std::isinf(value)) {  // Infinity check
        char* result = (char*)malloc(5);
        strcpy(result, "null");  // Infinity becomes null in JSON
        return result;
    }

    // Format the number
    snprintf(buffer, sizeof(buffer), "%.17g", value);

    size_t len = strlen(buffer);
    char* result = (char*)malloc(len + 1);
    strcpy(result, buffer);
    return result;
}

// Forward declarations for array functions (inside same extern C block)
int64_t nova_value_array_length(void* arr);
int64_t nova_value_array_at(void* arr, int64_t index);

// JSON.stringify(array) - OPTIMIZED - converts an array to a JSON string (ES5)
char* nova_json_stringify_array(void* arr) {
    if (!arr) {
        return (char*)JSON_NULL;
    }

    int64_t len = nova_value_array_length(arr);

    if (len == 0) {
        static const char* EMPTY_ARRAY = "[]";
        return (char*)EMPTY_ARRAY;
    }

    // Pre-calculate buffer size (estimate: 20 chars per number + commas)
    size_t bufferSize = 2 + len * 21;  // '[' + ']' + (number + ',') * len
    char* buffer = (char*)malloc(bufferSize);
    char* ptr = buffer;

    *ptr++ = '[';

    for (int64_t i = 0; i < len; i++) {
        int64_t value = nova_value_array_at(arr, i);

        // Add comma if not first element
        if (i > 0) {
            *ptr++ = ',';
        }

        // Use optimized fast_i64toa
        ptr += fast_i64toa(value, ptr);
    }

    *ptr++ = ']';
    *ptr = '\0';

    // Trim to actual size if we over-allocated significantly
    size_t actualSize = ptr - buffer;
    if (actualSize < bufferSize / 2) {
        char* result = (char*)malloc(actualSize + 1);
        memcpy(result, buffer, actualSize + 1);
        free(buffer);
        return result;
    }

    return buffer;
}

// Simple JSON parser state
struct JsonParseState {
    const char* input;
    size_t pos;
    size_t len;
};

// Skip whitespace
static void json_skip_whitespace(JsonParseState* state) {
    while (state->pos < state->len) {
        char c = state->input[state->pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            state->pos++;
        } else {
            break;
        }
    }
}

// Parse a JSON number
static int64_t json_parse_number(JsonParseState* state) {
    char buffer[64];
    size_t bufPos = 0;

    // Handle negative
    if (state->pos < state->len && state->input[state->pos] == '-') {
        buffer[bufPos++] = '-';
        state->pos++;
    }

    // Parse digits
    while (state->pos < state->len && bufPos < 63) {
        char c = state->input[state->pos];
        if (c >= '0' && c <= '9') {
            buffer[bufPos++] = c;
            state->pos++;
        } else if (c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') {
            buffer[bufPos++] = c;
            state->pos++;
        } else {
            break;
        }
    }
    buffer[bufPos] = '\0';

    return atoll(buffer);
}

// Parse a JSON string (returns allocated string without quotes)
static char* json_parse_string_internal(JsonParseState* state) {
    if (state->pos >= state->len || state->input[state->pos] != '"') {
        return nullptr;
    }
    state->pos++;  // Skip opening quote

    size_t start = state->pos;

    // Find end of string
    while (state->pos < state->len && state->input[state->pos] != '"') {
        if (state->input[state->pos] == '\\' && state->pos + 1 < state->len) {
            state->pos += 2;  // Skip escape sequence
        } else {
            state->pos++;
        }
    }

    size_t strLen = state->pos - start;

    // Allocate and copy string (simplified - no escape handling)
    char* result = (char*)malloc(strLen + 1);
    strncpy(result, state->input + start, strLen);
    result[strLen] = '\0';

    if (state->pos < state->len) {
        state->pos++;  // Skip closing quote
    }

    return result;
}

// JSON.parse(text) - parses a JSON string and returns the value (ES5)
void* nova_json_parse(const char* text) {
    if (!text) {
        return nullptr;
    }

    JsonParseState state;
    state.input = text;
    state.pos = 0;
    state.len = strlen(text);

    json_skip_whitespace(&state);

    if (state.pos >= state.len) {
        return nullptr;
    }

    char c = state.input[state.pos];

    // Parse based on first character
    if (c == '"') {
        // String
        return (void*)json_parse_string_internal(&state);
    } else if (c == '-' || (c >= '0' && c <= '9')) {
        // Number - return as pointer to int64_t
        int64_t* numPtr = (int64_t*)malloc(sizeof(int64_t));
        *numPtr = json_parse_number(&state);
        return (void*)numPtr;
    } else if (c == 't' && state.pos + 4 <= state.len &&
               strncmp(state.input + state.pos, "true", 4) == 0) {
        // true
        int64_t* boolPtr = (int64_t*)malloc(sizeof(int64_t));
        *boolPtr = 1;
        return (void*)boolPtr;
    } else if (c == 'f' && state.pos + 5 <= state.len &&
               strncmp(state.input + state.pos, "false", 5) == 0) {
        // false
        int64_t* boolPtr = (int64_t*)malloc(sizeof(int64_t));
        *boolPtr = 0;
        return (void*)boolPtr;
    } else if (c == 'n' && state.pos + 4 <= state.len &&
               strncmp(state.input + state.pos, "null", 4) == 0) {
        // null
        return nullptr;
    } else if (c == '[') {
        // Array - simplified, skip for now
        return nullptr;
    } else if (c == '{') {
        // Object - simplified, skip for now
        return nullptr;
    }

    return nullptr;
}

// JSON.parse() returning number directly (for simple number parsing)
int64_t nova_json_parse_number(const char* text) {
    if (!text) return 0;

    JsonParseState state;
    state.input = text;
    state.pos = 0;
    state.len = strlen(text);

    json_skip_whitespace(&state);

    if (state.pos >= state.len) return 0;

    char c = state.input[state.pos];
    if (c == '-' || (c >= '0' && c <= '9')) {
        return json_parse_number(&state);
    }

    return 0;
}

// JSON.parse() returning string directly
char* nova_json_parse_string(const char* text) {
    if (!text) return nullptr;

    JsonParseState state;
    state.input = text;
    state.pos = 0;
    state.len = strlen(text);

    json_skip_whitespace(&state);

    if (state.pos >= state.len) return nullptr;

    if (state.input[state.pos] == '"') {
        return json_parse_string_internal(&state);
    }

    return nullptr;
}

} // extern "C"

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

// Helper function to decode a base64 character
static int base64_decode_char(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1; // Invalid character or padding
}

// atob() - decodes a base64 encoded string (Web API)
char* nova_atob(const char* str) {
    if (!str) {
        char* result = (char*)malloc(1);
        result[0] = '\0';
        return result;
    }

    size_t len = strlen(str);
    if (len == 0) {
        char* result = (char*)malloc(1);
        result[0] = '\0';
        return result;
    }

    // Calculate output length (remove padding from count)
    size_t padding = 0;
    if (len >= 1 && str[len - 1] == '=') padding++;
    if (len >= 2 && str[len - 2] == '=') padding++;
    size_t outLen = (len / 4) * 3 - padding;

    char* result = (char*)malloc(outLen + 1);
    char* ptr = result;

    for (size_t i = 0; i < len; i += 4) {
        // Decode 4 base64 characters to 3 bytes
        int v0 = base64_decode_char(str[i]);
        int v1 = (i + 1 < len) ? base64_decode_char(str[i + 1]) : 0;
        int v2 = (i + 2 < len && str[i + 2] != '=') ? base64_decode_char(str[i + 2]) : 0;
        int v3 = (i + 3 < len && str[i + 3] != '=') ? base64_decode_char(str[i + 3]) : 0;

        // First byte
        *ptr++ = (v0 << 2) | (v1 >> 4);

        // Second byte (if not padding)
        if (i + 2 < len && str[i + 2] != '=') {
            *ptr++ = ((v1 & 0x0F) << 4) | (v2 >> 2);
        }

        // Third byte (if not padding)
        if (i + 3 < len && str[i + 3] != '=') {
            *ptr++ = ((v2 & 0x03) << 6) | v3;
        }
    }
    *ptr = '\0';

    return result;
}

// Helper function to check if character should NOT be encoded by encodeURI
static bool isURIUnencoded(unsigned char c) {
    // Unreserved characters (same as encodeURIComponent)
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) return true;
    if (c == '-' || c == '_' || c == '.' || c == '!' || c == '~' || c == '*' || c == '\'' || c == '(' || c == ')') return true;
    // Reserved characters (NOT encoded by encodeURI, but ARE encoded by encodeURIComponent)
    if (c == ';' || c == ',' || c == '/' || c == '?' || c == ':' || c == '@' || c == '&' || c == '=' || c == '+' || c == '$' || c == '#') return true;
    return false;
}

// encodeURI() - encodes a full URI, preserving URI-valid characters (ES3)
char* nova_encodeURI(const char* str) {
    if (!str) {
        char* result = (char*)malloc(1);
        result[0] = '\0';
        return result;
    }

    // First pass: calculate the output length
    size_t len = strlen(str);
    size_t outLen = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        if (isURIUnencoded(c)) {
            outLen++;
        } else {
            outLen += 3;  // %XX
        }
    }

    // Allocate result
    char* result = (char*)malloc(outLen + 1);
    char* ptr = result;

    // Second pass: encode
    const char* hexChars = "0123456789ABCDEF";
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        if (isURIUnencoded(c)) {
            *ptr++ = c;
        } else {
            *ptr++ = '%';
            *ptr++ = hexChars[(c >> 4) & 0x0F];
            *ptr++ = hexChars[c & 0x0F];
        }
    }
    *ptr = '\0';

    return result;
}

// decodeURI() - decodes a full URI (ES3)
// Note: The decoding logic is the same as decodeURIComponent - both decode %XX sequences
char* nova_decodeURI(const char* str) {
    if (!str) {
        char* result = (char*)malloc(1);
        result[0] = '\0';
        return result;
    }

    size_t len = strlen(str);
    // Output will be at most the same length as input (decoding shrinks)
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
                *ptr++ = str[i];  // Invalid hex, copy as-is
            }
        } else {
            *ptr++ = str[i];
        }
    }
    *ptr = '\0';

    return result;
}

// Exception handling functions - polling-based approach

// Begin a try block
void nova_try_begin() {
    g_try_depth++;
    g_exception_pending = false;
}

// End a try block
void nova_try_end() {
    g_try_depth--;
    if (g_try_depth < 0) {
        g_try_depth = 0;
    }
}

// Throw an exception - sets flag but doesn't longjmp
// The generated code must check and branch to catch block
void nova_throw(int64_t value) {
    g_exception_pending = true;
    g_exception_value = value;
    // If no try block is active, it's an uncaught exception
    if (g_try_depth <= 0) {
        std::cerr << "Uncaught exception: " << value << std::endl;
        exit(1);
    }
    // Otherwise, the exception will be caught by polling
}

// Check if exception is pending
int64_t nova_exception_pending() {
    return g_exception_pending ? 1 : 0;
}

// Get exception value
int64_t nova_get_exception() {
    return g_exception_value;
}

// Clear exception
void nova_clear_exception() {
    g_exception_pending = false;
    g_exception_value = 0;
}

// eval() - evaluates JavaScript code at runtime (ES1)
// AOT limitation: Only simple constant expressions are supported at compile time.
// For dynamic strings or complex expressions, this throws an EvalError.
int64_t nova_eval(const char* code) {
    if (!code) {
        return 0;  // undefined
    }

    std::string codeStr(code);

    // Trim whitespace
    size_t start = codeStr.find_first_not_of(" \t\n\r");
    size_t end = codeStr.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return 0;  // Empty string returns undefined
    }
    codeStr = codeStr.substr(start, end - start + 1);

    // Try to parse simple numeric literal
    bool isNumber = true;
    bool hasDecimal = false;
    size_t numStart = 0;

    if (!codeStr.empty() && codeStr[0] == '-') {
        numStart = 1;
    }

    for (size_t i = numStart; i < codeStr.size() && isNumber; i++) {
        if (codeStr[i] == '.') {
            if (hasDecimal) isNumber = false;
            else hasDecimal = true;
        } else if (!std::isdigit(codeStr[i])) {
            isNumber = false;
        }
    }

    if (isNumber && !codeStr.empty() && codeStr.size() > numStart) {
        try {
            return static_cast<int64_t>(std::stoll(codeStr));
        } catch (...) {
            // Fall through to error
        }
    }

    // Check for boolean literals
    if (codeStr == "true") return 1;
    if (codeStr == "false") return 0;
    if (codeStr == "null" || codeStr == "undefined") return 0;

    // Check for simple arithmetic: number op number
    for (char op : {'+', '-', '*', '/', '%'}) {
        size_t opPos = codeStr.find(op);
        if (opPos != std::string::npos && opPos > 0 && opPos < codeStr.size() - 1) {
            std::string leftStr = codeStr.substr(0, opPos);
            std::string rightStr = codeStr.substr(opPos + 1);

            // Trim
            size_t ls = leftStr.find_first_not_of(" \t");
            size_t le = leftStr.find_last_not_of(" \t");
            size_t rs = rightStr.find_first_not_of(" \t");
            size_t re = rightStr.find_last_not_of(" \t");

            if (ls != std::string::npos && le != std::string::npos &&
                rs != std::string::npos && re != std::string::npos) {
                leftStr = leftStr.substr(ls, le - ls + 1);
                rightStr = rightStr.substr(rs, re - rs + 1);

                try {
                    int64_t left = std::stoll(leftStr);
                    int64_t right = std::stoll(rightStr);

                    switch (op) {
                        case '+': return left + right;
                        case '-': return left - right;
                        case '*': return left * right;
                        case '/': return (right != 0) ? left / right : 0;
                        case '%': return (right != 0) ? left % right : 0;
                    }
                } catch (...) {
                    // Not valid numbers, fall through to error
                }
            }
        }
    }

    // Complex expression - throw EvalError
    std::cerr << "EvalError: eval() is not fully supported in AOT compilation. Code: " << code << std::endl;
    std::cerr << "Note: Only simple constant expressions (numbers, booleans, basic arithmetic) are supported." << std::endl;
    // For now, return 0 instead of crashing (in future could use exception system)
    return 0;
}

} // extern "C"