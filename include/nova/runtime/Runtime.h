#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace nova {
namespace runtime {

// Basic types for runtime
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using float32 = float;
using float64 = double;

// Object header for garbage collected objects
struct ObjectHeader {
    size_t size;
    uint32_t type_id;
    bool is_marked;
    ObjectHeader* next;
};

// Type identifiers for runtime objects
enum class TypeId : uint32_t {
    OBJECT = 0,
    ARRAY = 1,
    STRING = 2,
    FUNCTION = 3,
    CLOSURE = 4,
    USER_DEFINED = 1000
};

// Array structure
struct Array {
    ObjectHeader header;
    int64 length;
    int64 capacity;
    void* elements;
};

// String structure
struct String {
    ObjectHeader header;
    int64 length;
    char* data;
};

// Object structure
struct Object {
    ObjectHeader header;
    void* properties;
};

// Function pointer type
using FunctionPtr = void*(*)(void*, void**, size_t);

// Closure structure (function with captured environment)
struct Closure {
    ObjectHeader header;
    FunctionPtr function;
    void* environment;
};

// Memory management functions
void* allocate(size_t size, TypeId type_id);
void deallocate(void* ptr);
size_t get_object_size(void* ptr);
TypeId get_object_type(void* ptr);

// Garbage collection functions
void initialize_gc(size_t heap_size = 1024 * 1024);
void shutdown_gc();
void collect_garbage();
void add_root(void* ptr);
void remove_root(void* ptr);

// Array functions (pointer-based, for dynamic objects)
Array* create_array(int64 initial_capacity = 8);
void resize_array(Array* array, int64 new_capacity);
void* array_get(Array* array, int64 index);
void array_set(Array* array, int64 index, void* value);
int64 array_length(Array* array);
void array_push(Array* array, void* value);
void* array_pop(Array* array);
void* array_shift(Array* array);
void array_unshift(Array* array, void* value);

// Value array functions (value-based, for primitive types)
// These work directly with int64 values instead of pointers
struct ValueArray {
    ObjectHeader header;
    int64 length;
    int64 capacity;
    int64* elements;  // Direct value storage, not pointers
};

ValueArray* create_value_array(int64 initial_capacity);
ValueArray* convert_to_value_array(void* metadata_ptr);
void* create_metadata_from_value_array(ValueArray* array);
void resize_value_array(ValueArray* array, int64 new_capacity);
int64 value_array_get(ValueArray* array, int64 index);
void value_array_set(ValueArray* array, int64 index, int64 value);
int64 value_array_length(ValueArray* array);
void value_array_push(ValueArray* array, int64 value);
int64 value_array_pop(ValueArray* array);

const char* value_array_join(ValueArray* array, const char* delimiter);
ValueArray* value_array_concat(ValueArray* arr1, ValueArray* arr2);
ValueArray* value_array_slice(ValueArray* array, int64 start, int64 end);

// String array structure (for methods like String.split())
struct StringArray {
    ObjectHeader header;
    int64 length;
    int64 capacity;
    const char** elements;  // Array of string pointers
};

StringArray* create_string_array(int64 initial_capacity);
int64 value_array_shift(ValueArray* array);
void value_array_unshift(ValueArray* array, int64 value);

// String functions
String* create_string(const char* data);
String* create_string(const char* data, int64 length);
String* create_string(const std::string& str);
const char* string_data(String* str);
int64 string_length(String* str);
String* string_concat(String* a, String* b);
int32 string_compare(String* a, String* b);

// Object functions
Object* create_object();
void* object_get(Object* obj, const char* key);
void object_set(Object* obj, const char* key, void* value);
bool object_has(Object* obj, const char* key);
void object_delete(Object* obj, const char* key);

// Function and closure functions
Closure* create_closure(FunctionPtr function, void* environment = nullptr);
void* call_closure(Closure* closure, void** args, size_t arg_count);

// Utility functions
void print_value(void* value, TypeId type_id);
[[noreturn]] void panic(const char* message);
void assert_impl(bool condition, const char* message);

// Math functions
float64 math_abs(float64 x);
float64 math_sqrt(float64 x);
float64 math_pow(float64 base, float64 exp);
float64 math_sin(float64 x);
float64 math_cos(float64 x);
float64 math_tan(float64 x);
float64 math_log(float64 x);
float64 math_exp(float64 x);

// Integer math functions
int64 nova_math_sqrt_i64(int64 x);

// Random functions
void random_seed(uint32 seed);
uint32 random_next();
float64 random_float();

// Time functions
uint64 current_time_millis();
void sleep_ms(uint32 milliseconds);

// I/O functions
void print_string(const char* str);
void print_int(int64 value);
void print_float(float64 value);
void print_bool(bool value);
char* read_line();

// Async runtime functions
struct AsyncTask {
    std::function<void()> task;
    AsyncTask* next;
};

void async_init();
void async_shutdown();
void async_schedule(std::function<void()> task);
void async_wait_for_completion();

} // namespace runtime
} // namespace nova

// Test framework functions (bun:test compatible)
extern "C" {

// Test structure functions
void nova_describe(const char* name, void (*fn)());
void nova_test(const char* name, void (*fn)());
void nova_it(const char* name, void (*fn)());

// Expect functions
void* nova_expect_number(double value);
void* nova_expect_string(const char* value);
void* nova_expect_bool(int value);
void* nova_expect_not(void* exp);

// Matchers
void nova_expect_toBe_number(void* exp, double expected);
void nova_expect_toBe_string(void* exp, const char* expected);
void nova_expect_toBe_bool(void* exp, int expected);
void nova_expect_toEqual_number(void* exp, double expected);
void nova_expect_toEqual_string(void* exp, const char* expected);
void nova_expect_toEqual_bool(void* exp, int expected);
void nova_expect_toBeTruthy(void* exp);
void nova_expect_toBeFalsy(void* exp);
void nova_expect_toBeNull(void* exp);
void nova_expect_toBeDefined(void* exp);
void nova_expect_toBeUndefined(void* exp);
void nova_expect_toBeGreaterThan(void* exp, double expected);
void nova_expect_toBeGreaterThanOrEqual(void* exp, double expected);
void nova_expect_toBeLessThan(void* exp, double expected);
void nova_expect_toBeLessThanOrEqual(void* exp, double expected);
void nova_expect_toBeCloseTo(void* exp, double expected, int precision);
void nova_expect_toContain(void* exp, const char* item);
void nova_expect_toHaveLength(void* exp, int length);
void nova_expect_toMatch(void* exp, const char* pattern);
void nova_expect_toThrow(void* exp);

// Test utilities
void nova_test_summary();
int nova_test_exit_code();
void nova_test_reset();

}
