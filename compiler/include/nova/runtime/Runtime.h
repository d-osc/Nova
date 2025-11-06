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

// Array functions
Array* create_array(int64 initial_capacity = 8);
void resize_array(Array* array, int64 new_capacity);
void* array_get(Array* array, int64 index);
void array_set(Array* array, int64 index, void* value);
int64 array_length(Array* array);
void array_push(Array* array, void* value);
void* array_pop(Array* array);

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
void panic(const char* message);
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