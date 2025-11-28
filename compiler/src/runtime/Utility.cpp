#include "nova/runtime/Runtime.h"
#include <iostream>
#include <cmath>
#include <random>
#include <chrono>
#include <cstdio>
#include <cstdlib>

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

} // extern "C"