/**
 * nova:assert - Assert Module Implementation
 *
 * Provides assertion utilities for Nova programs.
 * Compatible with Node.js assert module.
 */

#include "nova/runtime/BuiltinModules.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <stdexcept>

namespace nova {
namespace runtime {
namespace assert {

// Helper to allocate and copy string
static char* allocString(const std::string& str) {
    char* result = (char*)malloc(str.length() + 1);
    if (result) {
        strcpy(result, str.c_str());
    }
    return result;
}

// AssertionError message
static char* lastError = nullptr;

static void setError(const char* message) {
    if (lastError) free(lastError);
    lastError = message ? allocString(message) : nullptr;
}

static void throwAssertionError(const char* message, const char* actual, const char* expected, const char* op) {
    char buffer[1024];
    if (actual && expected && op) {
        snprintf(buffer, sizeof(buffer), "AssertionError: %s\n  actual: %s\n  expected: %s\n  operator: %s",
                 message ? message : "Assertion failed", actual, expected, op);
    } else {
        snprintf(buffer, sizeof(buffer), "AssertionError: %s", message ? message : "Assertion failed");
    }
    setError(buffer);
    fprintf(stderr, "%s\n", buffer);
}

extern "C" {

// ============================================================================
// Basic Assertions
// ============================================================================

// assert(value[, message]) - assert value is truthy
int nova_assert(int value, const char* message) {
    if (!value) {
        throwAssertionError(message ? message : "The expression evaluated to a falsy value", nullptr, nullptr, nullptr);
        return 0;
    }
    return 1;
}

// assert.ok(value[, message]) - same as assert()
int nova_assert_ok(int value, const char* message) {
    return nova_assert(value, message);
}

// assert.fail([message]) - always fails
int nova_assert_fail(const char* message) {
    throwAssertionError(message ? message : "Failed", nullptr, nullptr, nullptr);
    return 0;
}

// ============================================================================
// Equality Assertions
// ============================================================================

// assert.equal(actual, expected[, message]) - == comparison
int nova_assert_equal(double actual, double expected, const char* message) {
    if (actual != expected) {
        char actualStr[64], expectedStr[64];
        snprintf(actualStr, sizeof(actualStr), "%g", actual);
        snprintf(expectedStr, sizeof(expectedStr), "%g", expected);
        throwAssertionError(message ? message : "Values are not equal", actualStr, expectedStr, "==");
        return 0;
    }
    return 1;
}

// assert.notEqual(actual, expected[, message]) - != comparison
int nova_assert_notEqual(double actual, double expected, const char* message) {
    if (actual == expected) {
        char actualStr[64], expectedStr[64];
        snprintf(actualStr, sizeof(actualStr), "%g", actual);
        snprintf(expectedStr, sizeof(expectedStr), "%g", expected);
        throwAssertionError(message ? message : "Values are equal", actualStr, expectedStr, "!=");
        return 0;
    }
    return 1;
}

// assert.strictEqual(actual, expected[, message]) - === comparison
int nova_assert_strictEqual(double actual, double expected, const char* message) {
    if (actual != expected) {
        char actualStr[64], expectedStr[64];
        snprintf(actualStr, sizeof(actualStr), "%g", actual);
        snprintf(expectedStr, sizeof(expectedStr), "%g", expected);
        throwAssertionError(message ? message : "Values are not strictly equal", actualStr, expectedStr, "===");
        return 0;
    }
    return 1;
}

// assert.notStrictEqual(actual, expected[, message]) - !== comparison
int nova_assert_notStrictEqual(double actual, double expected, const char* message) {
    if (actual == expected) {
        char actualStr[64], expectedStr[64];
        snprintf(actualStr, sizeof(actualStr), "%g", actual);
        snprintf(expectedStr, sizeof(expectedStr), "%g", expected);
        throwAssertionError(message ? message : "Values are strictly equal", actualStr, expectedStr, "!==");
        return 0;
    }
    return 1;
}

// ============================================================================
// String Equality Assertions
// ============================================================================

// assert.equal for strings
int nova_assert_equalString(const char* actual, const char* expected, const char* message) {
    int areEqual = (actual == expected) || (actual && expected && strcmp(actual, expected) == 0);
    if (!areEqual) {
        throwAssertionError(message ? message : "Strings are not equal",
                          actual ? actual : "null",
                          expected ? expected : "null", "==");
        return 0;
    }
    return 1;
}

// assert.notEqual for strings
int nova_assert_notEqualString(const char* actual, const char* expected, const char* message) {
    int areEqual = (actual == expected) || (actual && expected && strcmp(actual, expected) == 0);
    if (areEqual) {
        throwAssertionError(message ? message : "Strings are equal",
                          actual ? actual : "null",
                          expected ? expected : "null", "!=");
        return 0;
    }
    return 1;
}

// assert.strictEqual for strings
int nova_assert_strictEqualString(const char* actual, const char* expected, const char* message) {
    return nova_assert_equalString(actual, expected, message);
}

// assert.notStrictEqual for strings
int nova_assert_notStrictEqualString(const char* actual, const char* expected, const char* message) {
    return nova_assert_notEqualString(actual, expected, message);
}

// ============================================================================
// Deep Equality Assertions (simplified for primitives)
// ============================================================================

// assert.deepEqual - deep comparison (simplified)
int nova_assert_deepEqual(double actual, double expected, const char* message) {
    return nova_assert_equal(actual, expected, message);
}

// assert.notDeepEqual
int nova_assert_notDeepEqual(double actual, double expected, const char* message) {
    return nova_assert_notEqual(actual, expected, message);
}

// assert.deepStrictEqual
int nova_assert_deepStrictEqual(double actual, double expected, const char* message) {
    return nova_assert_strictEqual(actual, expected, message);
}

// assert.notDeepStrictEqual
int nova_assert_notDeepStrictEqual(double actual, double expected, const char* message) {
    return nova_assert_notStrictEqual(actual, expected, message);
}

// ============================================================================
// Type Assertions
// ============================================================================

// assert.ifError(value) - throws if value is truthy
int nova_assert_ifError(int value, const char* errorMessage) {
    if (value) {
        throwAssertionError(errorMessage ? errorMessage : "Got unwanted error", nullptr, nullptr, nullptr);
        return 0;
    }
    return 1;
}

// ============================================================================
// Comparison Assertions
// ============================================================================

// assert that actual > expected
int nova_assert_greater(double actual, double expected, const char* message) {
    if (!(actual > expected)) {
        char actualStr[64], expectedStr[64];
        snprintf(actualStr, sizeof(actualStr), "%g", actual);
        snprintf(expectedStr, sizeof(expectedStr), "%g", expected);
        throwAssertionError(message ? message : "Value is not greater", actualStr, expectedStr, ">");
        return 0;
    }
    return 1;
}

// assert that actual >= expected
int nova_assert_greaterOrEqual(double actual, double expected, const char* message) {
    if (!(actual >= expected)) {
        char actualStr[64], expectedStr[64];
        snprintf(actualStr, sizeof(actualStr), "%g", actual);
        snprintf(expectedStr, sizeof(expectedStr), "%g", expected);
        throwAssertionError(message ? message : "Value is not greater or equal", actualStr, expectedStr, ">=");
        return 0;
    }
    return 1;
}

// assert that actual < expected
int nova_assert_less(double actual, double expected, const char* message) {
    if (!(actual < expected)) {
        char actualStr[64], expectedStr[64];
        snprintf(actualStr, sizeof(actualStr), "%g", actual);
        snprintf(expectedStr, sizeof(expectedStr), "%g", expected);
        throwAssertionError(message ? message : "Value is not less", actualStr, expectedStr, "<");
        return 0;
    }
    return 1;
}

// assert that actual <= expected
int nova_assert_lessOrEqual(double actual, double expected, const char* message) {
    if (!(actual <= expected)) {
        char actualStr[64], expectedStr[64];
        snprintf(actualStr, sizeof(actualStr), "%g", actual);
        snprintf(expectedStr, sizeof(expectedStr), "%g", expected);
        throwAssertionError(message ? message : "Value is not less or equal", actualStr, expectedStr, "<=");
        return 0;
    }
    return 1;
}

// ============================================================================
// String Pattern Assertions
// ============================================================================

// assert.match(string, regexp[, message]) - simplified pattern match
int nova_assert_match(const char* str, const char* pattern, const char* message) {
    if (!str || !pattern) {
        throwAssertionError(message ? message : "Invalid arguments to match", str, pattern, "match");
        return 0;
    }
    // Simple substring match (not full regex)
    if (strstr(str, pattern) == nullptr) {
        throwAssertionError(message ? message : "String does not match pattern", str, pattern, "match");
        return 0;
    }
    return 1;
}

// assert.doesNotMatch(string, regexp[, message])
int nova_assert_doesNotMatch(const char* str, const char* pattern, const char* message) {
    if (!str || !pattern) {
        return 1; // null doesn't match anything
    }
    if (strstr(str, pattern) != nullptr) {
        throwAssertionError(message ? message : "String matches pattern", str, pattern, "doesNotMatch");
        return 0;
    }
    return 1;
}

// ============================================================================
// Throws/Rejects Assertions (simplified)
// ============================================================================

// assert.throws - check if function throws
// Note: In C++, we can't directly call JS functions, so this is a placeholder
int nova_assert_throws(int didThrow, const char* message) {
    if (!didThrow) {
        throwAssertionError(message ? message : "Missing expected exception", nullptr, nullptr, "throws");
        return 0;
    }
    return 1;
}

// assert.doesNotThrow
int nova_assert_doesNotThrow(int didThrow, const char* message) {
    if (didThrow) {
        throwAssertionError(message ? message : "Got unwanted exception", nullptr, nullptr, "doesNotThrow");
        return 0;
    }
    return 1;
}

// assert.rejects - for async (placeholder)
int nova_assert_rejects(int didReject, const char* message) {
    return nova_assert_throws(didReject, message);
}

// assert.doesNotReject
int nova_assert_doesNotReject(int didReject, const char* message) {
    return nova_assert_doesNotThrow(didReject, message);
}

// ============================================================================
// Null/Undefined Assertions
// ============================================================================

// assert that value is null/undefined
int nova_assert_isNull(const void* value, const char* message) {
    if (value != nullptr) {
        throwAssertionError(message ? message : "Expected null", "non-null", "null", "===");
        return 0;
    }
    return 1;
}

// assert that value is not null/undefined
int nova_assert_isNotNull(const void* value, const char* message) {
    if (value == nullptr) {
        throwAssertionError(message ? message : "Expected non-null", "null", "non-null", "!==");
        return 0;
    }
    return 1;
}

// ============================================================================
// Type Check Assertions
// ============================================================================

// assert.isTrue
int nova_assert_isTrue(int value, const char* message) {
    if (value != 1) {
        throwAssertionError(message ? message : "Expected true", value ? "truthy" : "false", "true", "===");
        return 0;
    }
    return 1;
}

// assert.isFalse
int nova_assert_isFalse(int value, const char* message) {
    if (value != 0) {
        throwAssertionError(message ? message : "Expected false", "truthy", "false", "===");
        return 0;
    }
    return 1;
}

// assert.isNaN
int nova_assert_isNaN(double value, const char* message) {
    if (!std::isnan(value)) {
        char actualStr[64];
        snprintf(actualStr, sizeof(actualStr), "%g", value);
        throwAssertionError(message ? message : "Expected NaN", actualStr, "NaN", "===");
        return 0;
    }
    return 1;
}

// assert.isNotNaN
int nova_assert_isNotNaN(double value, const char* message) {
    if (std::isnan(value)) {
        throwAssertionError(message ? message : "Expected not NaN", "NaN", "not NaN", "!==");
        return 0;
    }
    return 1;
}

// assert.isFinite
int nova_assert_isFinite(double value, const char* message) {
    if (!std::isfinite(value)) {
        char actualStr[64];
        snprintf(actualStr, sizeof(actualStr), "%g", value);
        throwAssertionError(message ? message : "Expected finite number", actualStr, "finite", "===");
        return 0;
    }
    return 1;
}

// ============================================================================
// Array Assertions
// ============================================================================

// assert array includes value
int nova_assert_includes(const char* haystack, const char* needle, const char* message) {
    if (!haystack || !needle) {
        throwAssertionError(message ? message : "Invalid arguments", nullptr, nullptr, "includes");
        return 0;
    }
    if (strstr(haystack, needle) == nullptr) {
        throwAssertionError(message ? message : "Value not found", haystack, needle, "includes");
        return 0;
    }
    return 1;
}

// assert array does not include value
int nova_assert_notIncludes(const char* haystack, const char* needle, const char* message) {
    if (!haystack || !needle) {
        return 1;
    }
    if (strstr(haystack, needle) != nullptr) {
        throwAssertionError(message ? message : "Value found but should not be", haystack, needle, "notIncludes");
        return 0;
    }
    return 1;
}

// ============================================================================
// Utility Functions
// ============================================================================

// Get last assertion error message
char* nova_assert_getLastError() {
    return lastError ? allocString(lastError) : nullptr;
}

// Clear last error
void nova_assert_clearError() {
    if (lastError) {
        free(lastError);
        lastError = nullptr;
    }
}

// Strict mode (default: true)
static int strictMode = 1;

void nova_assert_setStrict(int strict) {
    strictMode = strict ? 1 : 0;
}

int nova_assert_getStrict() {
    return strictMode;
}

// AssertionError class support
int nova_assert_AssertionError_code() {
    return 1; // ERR_ASSERTION
}

char* nova_assert_AssertionError_name() {
    return allocString("AssertionError");
}

} // extern "C"

} // namespace assert
} // namespace runtime
} // namespace nova
