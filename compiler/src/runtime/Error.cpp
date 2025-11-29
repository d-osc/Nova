// Nova Runtime - JavaScript Error Types
// Implements Error, TypeError, RangeError, ReferenceError, SyntaxError, URIError, AggregateError

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

extern "C" {

// Forward declarations for exception handling (from Utility.cpp)
extern void nova_throw(int64_t value);

// ============================================================================
// Error Type IDs
// ============================================================================
enum NovaErrorType {
    ERROR_TYPE_ERROR = 1,
    ERROR_TYPE_RANGE_ERROR = 2,
    ERROR_TYPE_REFERENCE_ERROR = 3,
    ERROR_TYPE_SYNTAX_ERROR = 4,
    ERROR_TYPE_TYPE_ERROR = 5,
    ERROR_TYPE_URI_ERROR = 6,
    ERROR_TYPE_AGGREGATE_ERROR = 7,
    ERROR_TYPE_INTERNAL_ERROR = 8,
    ERROR_TYPE_EVAL_ERROR = 9
};

// ============================================================================
// Error Object Structure
// ============================================================================
struct NovaError {
    int64_t errorType;      // Error type ID
    char* name;             // Error name (e.g., "TypeError")
    char* message;          // Error message
    char* stack;            // Stack trace (simplified)
    char* fileName;         // Source file name
    int64_t lineNumber;     // Line number
    int64_t columnNumber;   // Column number
    // For AggregateError
    void** errors;          // Array of nested errors
    int64_t errorCount;     // Number of nested errors
};

// ============================================================================
// Helper Functions
// ============================================================================

static char* strdup_safe(const char* str) {
    if (!str) return strdup("");
    return strdup(str);
}

static char* create_stack_trace(const char* errorName, const char* message) {
    // Create a simple stack trace string
    // In a full implementation, this would capture the actual call stack
    char* stack = (char*)malloc(512);
    snprintf(stack, 512, "%s: %s\n    at <anonymous>:0:0",
             errorName ? errorName : "Error",
             message ? message : "");
    return stack;
}

// ============================================================================
// Error Creation Functions
// ============================================================================

// Create base Error
void* nova_error_create(const char* message) {
    NovaError* error = new NovaError();
    error->errorType = ERROR_TYPE_ERROR;
    error->name = strdup("Error");
    error->message = strdup_safe(message);
    error->stack = create_stack_trace("Error", message);
    error->fileName = strdup("");
    error->lineNumber = 0;
    error->columnNumber = 0;
    error->errors = nullptr;
    error->errorCount = 0;
    return error;
}

// Create TypeError
void* nova_type_error_create(const char* message) {
    NovaError* error = new NovaError();
    error->errorType = ERROR_TYPE_TYPE_ERROR;
    error->name = strdup("TypeError");
    error->message = strdup_safe(message);
    error->stack = create_stack_trace("TypeError", message);
    error->fileName = strdup("");
    error->lineNumber = 0;
    error->columnNumber = 0;
    error->errors = nullptr;
    error->errorCount = 0;
    return error;
}

// Create RangeError
void* nova_range_error_create(const char* message) {
    NovaError* error = new NovaError();
    error->errorType = ERROR_TYPE_RANGE_ERROR;
    error->name = strdup("RangeError");
    error->message = strdup_safe(message);
    error->stack = create_stack_trace("RangeError", message);
    error->fileName = strdup("");
    error->lineNumber = 0;
    error->columnNumber = 0;
    error->errors = nullptr;
    error->errorCount = 0;
    return error;
}

// Create ReferenceError
void* nova_reference_error_create(const char* message) {
    NovaError* error = new NovaError();
    error->errorType = ERROR_TYPE_REFERENCE_ERROR;
    error->name = strdup("ReferenceError");
    error->message = strdup_safe(message);
    error->stack = create_stack_trace("ReferenceError", message);
    error->fileName = strdup("");
    error->lineNumber = 0;
    error->columnNumber = 0;
    error->errors = nullptr;
    error->errorCount = 0;
    return error;
}

// Create SyntaxError
void* nova_syntax_error_create(const char* message) {
    NovaError* error = new NovaError();
    error->errorType = ERROR_TYPE_SYNTAX_ERROR;
    error->name = strdup("SyntaxError");
    error->message = strdup_safe(message);
    error->stack = create_stack_trace("SyntaxError", message);
    error->fileName = strdup("");
    error->lineNumber = 0;
    error->columnNumber = 0;
    error->errors = nullptr;
    error->errorCount = 0;
    return error;
}

// Create URIError
void* nova_uri_error_create(const char* message) {
    NovaError* error = new NovaError();
    error->errorType = ERROR_TYPE_URI_ERROR;
    error->name = strdup("URIError");
    error->message = strdup_safe(message);
    error->stack = create_stack_trace("URIError", message);
    error->fileName = strdup("");
    error->lineNumber = 0;
    error->columnNumber = 0;
    error->errors = nullptr;
    error->errorCount = 0;
    return error;
}

// Create InternalError
void* nova_internal_error_create(const char* message) {
    NovaError* error = new NovaError();
    error->errorType = ERROR_TYPE_INTERNAL_ERROR;
    error->name = strdup("InternalError");
    error->message = strdup_safe(message);
    error->stack = create_stack_trace("InternalError", message);
    error->fileName = strdup("");
    error->lineNumber = 0;
    error->columnNumber = 0;
    error->errors = nullptr;
    error->errorCount = 0;
    return error;
}

// Create EvalError
void* nova_eval_error_create(const char* message) {
    NovaError* error = new NovaError();
    error->errorType = ERROR_TYPE_EVAL_ERROR;
    error->name = strdup("EvalError");
    error->message = strdup_safe(message);
    error->stack = create_stack_trace("EvalError", message);
    error->fileName = strdup("");
    error->lineNumber = 0;
    error->columnNumber = 0;
    error->errors = nullptr;
    error->errorCount = 0;
    return error;
}

// Create AggregateError with array of errors
void* nova_aggregate_error_create(const char* message, void** errors, int64_t errorCount) {
    NovaError* error = new NovaError();
    error->errorType = ERROR_TYPE_AGGREGATE_ERROR;
    error->name = strdup("AggregateError");
    error->message = strdup_safe(message);
    error->stack = create_stack_trace("AggregateError", message);
    error->fileName = strdup("");
    error->lineNumber = 0;
    error->columnNumber = 0;

    // Copy the errors array
    if (errors && errorCount > 0) {
        error->errors = (void**)malloc(sizeof(void*) * errorCount);
        memcpy(error->errors, errors, sizeof(void*) * errorCount);
        error->errorCount = errorCount;
    } else {
        error->errors = nullptr;
        error->errorCount = 0;
    }
    return error;
}

// ============================================================================
// Error Property Getters
// ============================================================================

const char* nova_error_get_name(void* errorPtr) {
    if (!errorPtr) return "Error";
    NovaError* error = static_cast<NovaError*>(errorPtr);
    return error->name ? error->name : "Error";
}

const char* nova_error_get_message(void* errorPtr) {
    if (!errorPtr) return "";
    NovaError* error = static_cast<NovaError*>(errorPtr);
    return error->message ? error->message : "";
}

const char* nova_error_get_stack(void* errorPtr) {
    if (!errorPtr) return "";
    NovaError* error = static_cast<NovaError*>(errorPtr);
    return error->stack ? error->stack : "";
}

int64_t nova_error_get_type(void* errorPtr) {
    if (!errorPtr) return ERROR_TYPE_ERROR;
    NovaError* error = static_cast<NovaError*>(errorPtr);
    return error->errorType;
}

// ============================================================================
// Error toString
// ============================================================================

const char* nova_error_toString(void* errorPtr) {
    if (!errorPtr) return "Error";
    NovaError* error = static_cast<NovaError*>(errorPtr);

    // Format: "ErrorName: message"
    const char* name = error->name ? error->name : "Error";
    const char* msg = error->message ? error->message : "";

    size_t len = strlen(name) + 2 + strlen(msg) + 1;
    char* result = (char*)malloc(len);

    if (strlen(msg) > 0) {
        snprintf(result, len, "%s: %s", name, msg);
    } else {
        snprintf(result, len, "%s", name);
    }

    return result;
}

// ============================================================================
// Throw Functions - Create and throw errors in one call
// ============================================================================

void nova_throw_error(const char* message) {
    void* error = nova_error_create(message);
    std::cerr << "Uncaught Error: " << (message ? message : "") << std::endl;
    nova_throw(reinterpret_cast<int64_t>(error));
}

void nova_throw_type_error(const char* message) {
    void* error = nova_type_error_create(message);
    std::cerr << "Uncaught TypeError: " << (message ? message : "") << std::endl;
    nova_throw(reinterpret_cast<int64_t>(error));
}

void nova_throw_range_error(const char* message) {
    void* error = nova_range_error_create(message);
    std::cerr << "Uncaught RangeError: " << (message ? message : "") << std::endl;
    nova_throw(reinterpret_cast<int64_t>(error));
}

void nova_throw_reference_error(const char* message) {
    void* error = nova_reference_error_create(message);
    std::cerr << "Uncaught ReferenceError: " << (message ? message : "") << std::endl;
    nova_throw(reinterpret_cast<int64_t>(error));
}

void nova_throw_syntax_error(const char* message) {
    void* error = nova_syntax_error_create(message);
    std::cerr << "Uncaught SyntaxError: " << (message ? message : "") << std::endl;
    nova_throw(reinterpret_cast<int64_t>(error));
}

void nova_throw_uri_error(const char* message) {
    void* error = nova_uri_error_create(message);
    std::cerr << "Uncaught URIError: " << (message ? message : "") << std::endl;
    nova_throw(reinterpret_cast<int64_t>(error));
}

void nova_throw_internal_error(const char* message) {
    void* error = nova_internal_error_create(message);
    std::cerr << "Uncaught InternalError: " << (message ? message : "") << std::endl;
    nova_throw(reinterpret_cast<int64_t>(error));
}

void nova_throw_aggregate_error(const char* message) {
    void* error = nova_aggregate_error_create(message, nullptr, 0);
    std::cerr << "Uncaught AggregateError: " << (message ? message : "") << std::endl;
    nova_throw(reinterpret_cast<int64_t>(error));
}

// ============================================================================
// Specific Error Message Functions
// These throw common errors with predefined messages
// ============================================================================

// TypeError messages
void nova_throw_not_a_function(const char* name) {
    char msg[256];
    snprintf(msg, sizeof(msg), "%s is not a function", name ? name : "undefined");
    nova_throw_type_error(msg);
}

void nova_throw_not_a_constructor(const char* name) {
    char msg[256];
    snprintf(msg, sizeof(msg), "%s is not a constructor", name ? name : "undefined");
    nova_throw_type_error(msg);
}

void nova_throw_cannot_read_property(const char* prop, const char* type) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Cannot read property '%s' of %s",
             prop ? prop : "", type ? type : "undefined");
    nova_throw_type_error(msg);
}

void nova_throw_cannot_set_property(const char* prop, const char* type) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Cannot set property '%s' of %s",
             prop ? prop : "", type ? type : "undefined");
    nova_throw_type_error(msg);
}

void nova_throw_not_iterable(const char* name) {
    char msg[256];
    snprintf(msg, sizeof(msg), "%s is not iterable", name ? name : "object");
    nova_throw_type_error(msg);
}

void nova_throw_cannot_convert_to_bigint(const char* value) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Cannot convert %s to a BigInt", value ? value : "value");
    nova_throw_type_error(msg);
}

void nova_throw_invalid_instanceof(const char* name) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Right-hand side of 'instanceof' is not an object");
    nova_throw_type_error(msg);
}

void nova_throw_assignment_to_const(const char* name) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Assignment to constant variable '%s'", name ? name : "");
    nova_throw_type_error(msg);
}

// RangeError messages
void nova_throw_invalid_array_length() {
    nova_throw_range_error("Invalid array length");
}

void nova_throw_invalid_date() {
    nova_throw_range_error("Invalid Date");
}

void nova_throw_precision_out_of_range() {
    nova_throw_range_error("precision is out of range");
}

void nova_throw_radix_out_of_range() {
    nova_throw_range_error("radix must be an integer at least 2 and no greater than 36");
}

void nova_throw_repeat_count_negative() {
    nova_throw_range_error("repeat count must be non-negative");
}

void nova_throw_repeat_count_infinity() {
    nova_throw_range_error("repeat count must be less than infinity");
}

void nova_throw_invalid_code_point(int64_t codePoint) {
    char msg[256];
    snprintf(msg, sizeof(msg), "%lld is not a valid code point", (long long)codePoint);
    nova_throw_range_error(msg);
}

void nova_throw_bigint_division_by_zero() {
    nova_throw_range_error("BigInt division by zero");
}

void nova_throw_bigint_negative_exponent() {
    nova_throw_range_error("BigInt negative exponent");
}

void nova_throw_maximum_call_stack() {
    nova_throw_range_error("Maximum call stack size exceeded");
}

// ReferenceError messages
void nova_throw_not_defined(const char* name) {
    char msg[256];
    snprintf(msg, sizeof(msg), "%s is not defined", name ? name : "variable");
    nova_throw_reference_error(msg);
}

void nova_throw_cannot_access_before_init(const char* name) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Cannot access '%s' before initialization", name ? name : "variable");
    nova_throw_reference_error(msg);
}

void nova_throw_super_not_called() {
    nova_throw_reference_error("Must call super constructor in derived class before accessing 'this' or returning from derived constructor");
}

void nova_throw_assignment_to_undeclared(const char* name) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Assignment to undeclared variable '%s'", name ? name : "");
    nova_throw_reference_error(msg);
}

// SyntaxError messages (typically caught at compile time, but some are runtime)
void nova_throw_invalid_json(const char* detail) {
    char msg[256];
    snprintf(msg, sizeof(msg), "JSON.parse: %s", detail ? detail : "unexpected token");
    nova_throw_syntax_error(msg);
}

void nova_throw_invalid_regex(const char* pattern) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Invalid regular expression: %s", pattern ? pattern : "");
    nova_throw_syntax_error(msg);
}

// URIError messages
void nova_throw_malformed_uri() {
    nova_throw_uri_error("URI malformed");
}

void nova_throw_malformed_uri_sequence() {
    nova_throw_uri_error("malformed URI sequence");
}

// InternalError messages
void nova_throw_too_much_recursion() {
    nova_throw_internal_error("too much recursion");
}

void nova_throw_out_of_memory() {
    nova_throw_internal_error("out of memory");
}

// ============================================================================
// Error Checking Functions (for runtime validation)
// ============================================================================

// Check if value is null or undefined and throw TypeError if so
void nova_check_not_nullish(void* value, const char* context) {
    if (value == nullptr || value == (void*)-1) {  // -1 often represents undefined
        char msg[256];
        snprintf(msg, sizeof(msg), "Cannot read properties of %s (%s)",
                 value == nullptr ? "null" : "undefined", context ? context : "");
        nova_throw_type_error(msg);
    }
}

// Check array index bounds
void nova_check_array_bounds(int64_t index, int64_t length) {
    if (index < 0 || index >= length) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Index %lld out of bounds for array of length %lld",
                 (long long)index, (long long)length);
        nova_throw_range_error(msg);
    }
}

// Check for valid array length
void nova_check_array_length(int64_t length) {
    if (length < 0 || length > 4294967295LL) {  // Max safe array length
        nova_throw_invalid_array_length();
    }
}

// Check if value is callable
void nova_check_callable(void* value, const char* name) {
    // In a full implementation, this would check the value's type
    if (value == nullptr) {
        nova_throw_not_a_function(name);
    }
}

// ============================================================================
// Error Free Function
// ============================================================================

void nova_error_free(void* errorPtr) {
    if (!errorPtr) return;
    NovaError* error = static_cast<NovaError*>(errorPtr);

    if (error->name) free(error->name);
    if (error->message) free(error->message);
    if (error->stack) free(error->stack);
    if (error->fileName) free(error->fileName);
    if (error->errors) free(error->errors);

    delete error;
}

// ============================================================================
// instanceof check for Error types
// ============================================================================

int64_t nova_is_error(void* value) {
    if (!value) return 0;
    NovaError* error = static_cast<NovaError*>(value);
    return (error->errorType >= ERROR_TYPE_ERROR && error->errorType <= ERROR_TYPE_EVAL_ERROR) ? 1 : 0;
}

int64_t nova_is_type_error(void* value) {
    if (!value) return 0;
    NovaError* error = static_cast<NovaError*>(value);
    return error->errorType == ERROR_TYPE_TYPE_ERROR ? 1 : 0;
}

int64_t nova_is_range_error(void* value) {
    if (!value) return 0;
    NovaError* error = static_cast<NovaError*>(value);
    return error->errorType == ERROR_TYPE_RANGE_ERROR ? 1 : 0;
}

int64_t nova_is_reference_error(void* value) {
    if (!value) return 0;
    NovaError* error = static_cast<NovaError*>(value);
    return error->errorType == ERROR_TYPE_REFERENCE_ERROR ? 1 : 0;
}

int64_t nova_is_syntax_error(void* value) {
    if (!value) return 0;
    NovaError* error = static_cast<NovaError*>(value);
    return error->errorType == ERROR_TYPE_SYNTAX_ERROR ? 1 : 0;
}

int64_t nova_is_uri_error(void* value) {
    if (!value) return 0;
    NovaError* error = static_cast<NovaError*>(value);
    return error->errorType == ERROR_TYPE_URI_ERROR ? 1 : 0;
}

int64_t nova_is_aggregate_error(void* value) {
    if (!value) return 0;
    NovaError* error = static_cast<NovaError*>(value);
    return error->errorType == ERROR_TYPE_AGGREGATE_ERROR ? 1 : 0;
}

// ============================================================================
// AggregateError specific functions
// ============================================================================

// Get errors array from AggregateError
void* nova_aggregate_error_get_errors(void* errorPtr) {
    if (!errorPtr) return nullptr;
    NovaError* error = static_cast<NovaError*>(errorPtr);
    if (error->errorType != ERROR_TYPE_AGGREGATE_ERROR) return nullptr;
    return error->errors;
}

// Get error count from AggregateError
int64_t nova_aggregate_error_get_count(void* errorPtr) {
    if (!errorPtr) return 0;
    NovaError* error = static_cast<NovaError*>(errorPtr);
    if (error->errorType != ERROR_TYPE_AGGREGATE_ERROR) return 0;
    return error->errorCount;
}

} // extern "C"
