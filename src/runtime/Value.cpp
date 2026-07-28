#include "nova/runtime/Value.h"

#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

namespace {

using namespace nova::runtime;

JSValue double_bits(double value) {
    if (std::isnan(value)) return JS_VALUE_CANONICAL_NAN;
    JSValue bits = 0;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

double bits_double(JSValue value) {
    double number = 0.0;
    std::memcpy(&number, &value, sizeof(number));
    return number;
}

bool is_tagged(JSValue value) {
    const JSValue tag = value & JS_VALUE_TAG_MASK;
    return tag >= JS_VALUE_UNDEFINED && tag <= JS_VALUE_OBJECT_TAG;
}

bool is_number(JSValue value) {
    return !is_tagged(value);
}

const char* string_payload(JSValue value) {
    return reinterpret_cast<const char*>(
        static_cast<std::uintptr_t>(value & JS_VALUE_PAYLOAD_MASK));
}

JSValue pointer_value(JSValue tag, const void* pointer) {
    return tag | (static_cast<JSValue>(reinterpret_cast<std::uintptr_t>(pointer)) &
                  JS_VALUE_PAYLOAD_MASK);
}

double string_to_number(const char* text) {
    if (!text) return std::numeric_limits<double>::quiet_NaN();
    while (*text && std::isspace(static_cast<unsigned char>(*text))) ++text;
    if (!*text) return 0.0;
    // Spec 7.1.4.1: StringToNumber with full grammar including Infinity and 0x.
    if (std::strncmp(text, "Infinity", 8) == 0) {
        const char* rest = text + 8;
        while (*rest && std::isspace(static_cast<unsigned char>(*rest))) ++rest;
        if (*rest == '\0') return std::numeric_limits<double>::infinity();
    }
    if (std::strncmp(text, "+Infinity", 9) == 0) {
        const char* rest = text + 9;
        while (*rest && std::isspace(static_cast<unsigned char>(*rest))) ++rest;
        if (*rest == '\0') return std::numeric_limits<double>::infinity();
    }
    if (std::strncmp(text, "-Infinity", 9) == 0) {
        const char* rest = text + 9;
        while (*rest && std::isspace(static_cast<unsigned char>(*rest))) ++rest;
        if (*rest == '\0') return -std::numeric_limits<double>::infinity();
    }
    // Hex literal: 0x... or 0X... (no sign allowed by spec for hex).
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        const char* hexStart = text + 2;
        char* end = nullptr;
        const unsigned long long value = std::strtoull(hexStart, &end, 16);
        if (end == hexStart) return std::numeric_limits<double>::quiet_NaN();
        while (*end && std::isspace(static_cast<unsigned char>(*end))) ++end;
        if (*end != '\0') return std::numeric_limits<double>::quiet_NaN();
        return static_cast<double>(value);
    }
    char* end = nullptr;
    const double number = std::strtod(text, &end);
    if (end == text) return std::numeric_limits<double>::quiet_NaN();
    while (*end && std::isspace(static_cast<unsigned char>(*end))) ++end;
    return *end == '\0' ? number : std::numeric_limits<double>::quiet_NaN();
}

double value_to_number(JSValue value) {
    if (is_number(value)) return bits_double(value);
    if (value == JS_VALUE_NULL) return 0.0;
    if (value == JS_VALUE_TRUE) return 1.0;
    if (value == JS_VALUE_FALSE) return 0.0;
    if (js_value_has_tag(value, JS_VALUE_STRING_TAG)) {
        return string_to_number(string_payload(value));
    }
    return std::numeric_limits<double>::quiet_NaN();
}

std::string value_to_string(JSValue value) {
    if (value == JS_VALUE_UNDEFINED) return "undefined";
    if (value == JS_VALUE_NULL) return "null";
    if (value == JS_VALUE_FALSE) return "false";
    if (value == JS_VALUE_TRUE) return "true";
    if (js_value_has_tag(value, JS_VALUE_STRING_TAG)) {
        const char* text = string_payload(value);
        return text ? text : "";
    }
    if (js_value_has_tag(value, JS_VALUE_OBJECT_TAG)) return "[object Object]";
    char buffer[64] = {};
    std::snprintf(buffer, sizeof(buffer), "%.15g", bits_double(value));
    return buffer;
}

} // namespace

extern "C" {

std::uint64_t nova_value_from_i64(std::int64_t value) {
    return double_bits(static_cast<double>(value));
}

std::uint64_t nova_value_from_f64(double value) {
    return double_bits(value);
}

std::uint64_t nova_value_from_bool(std::int64_t value) {
    return value ? JS_VALUE_TRUE : JS_VALUE_FALSE;
}

std::uint64_t nova_value_from_string(const char* value) {
    return pointer_value(JS_VALUE_STRING_TAG, value);
}

std::uint64_t nova_value_from_object(void* value) {
    return value ? pointer_value(JS_VALUE_OBJECT_TAG, value) : JS_VALUE_NULL;
}

void* nova_value_to_object(std::uint64_t value) {
    if (!js_value_has_tag(value, JS_VALUE_OBJECT_TAG)) return nullptr;
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(
        value & JS_VALUE_PAYLOAD_MASK));
}

const char* nova_value_to_string_ptr(std::uint64_t value) {
    if (js_value_has_tag(value, JS_VALUE_STRING_TAG)) {
        return string_payload(value);
    }
    static thread_local std::string converted;
    converted = value_to_string(value);
    return converted.c_str();
}

// Helper for the `in` operator: convert an integer key like 0, 1, 42 into a
// heap-allocated C string. The lifetime is owned by the caller (the HIR
// `in` path is a transient lookup), so we leak intentionally like other key
// material generated during property access.
const char* nova_value_key_to_string(std::int64_t value) {
    std::string s = std::to_string(value);
    char* buf = new char[s.size() + 1];
    std::memcpy(buf, s.c_str(), s.size() + 1);
    return buf;
}

std::int64_t nova_value_to_boolean(std::uint64_t value) {
    if (value == JS_VALUE_UNDEFINED || value == JS_VALUE_NULL ||
        value == JS_VALUE_FALSE) return 0;
    if (value == JS_VALUE_TRUE || js_value_has_tag(value, JS_VALUE_STRING_TAG) ||
        js_value_has_tag(value, JS_VALUE_OBJECT_TAG)) {
        if (js_value_has_tag(value, JS_VALUE_STRING_TAG)) {
            const char* text = string_payload(value);
            return text && *text ? 1 : 0;
        }
        return 1;
    }
    const double number = bits_double(value);
    return number != 0.0 && !std::isnan(number) ? 1 : 0;
}

std::int64_t nova_value_is_nullish(std::uint64_t value) {
    return value == JS_VALUE_UNDEFINED || value == JS_VALUE_NULL ? 1 : 0;
}

std::int64_t nova_value_is_undefined(std::uint64_t value) {
    return value == JS_VALUE_UNDEFINED ? 1 : 0;
}

std::int64_t nova_value_strict_equal(std::uint64_t lhs, std::uint64_t rhs) {
    if (is_number(lhs) && is_number(rhs)) {
        return bits_double(lhs) == bits_double(rhs) ? 1 : 0;
    }
    if (js_value_has_tag(lhs, JS_VALUE_STRING_TAG) &&
        js_value_has_tag(rhs, JS_VALUE_STRING_TAG)) {
        const char* left = string_payload(lhs);
        const char* right = string_payload(rhs);
        if (!left || !right) return left == right ? 1 : 0;
        return std::strcmp(left, right) == 0 ? 1 : 0;
    }
    return lhs == rhs ? 1 : 0;
}

std::int64_t nova_value_abstract_equal(std::uint64_t lhs, std::uint64_t rhs) {
    if (nova_value_strict_equal(lhs, rhs)) return 1;
    const bool lhsNullish = lhs == JS_VALUE_NULL || lhs == JS_VALUE_UNDEFINED;
    const bool rhsNullish = rhs == JS_VALUE_NULL || rhs == JS_VALUE_UNDEFINED;
    if (lhsNullish || rhsNullish) return lhsNullish && rhsNullish ? 1 : 0;

    if (lhs == JS_VALUE_TRUE || lhs == JS_VALUE_FALSE) {
        lhs = double_bits(lhs == JS_VALUE_TRUE ? 1.0 : 0.0);
    }
    if (rhs == JS_VALUE_TRUE || rhs == JS_VALUE_FALSE) {
        rhs = double_bits(rhs == JS_VALUE_TRUE ? 1.0 : 0.0);
    }
    if (is_number(lhs) && js_value_has_tag(rhs, JS_VALUE_STRING_TAG)) {
        const double converted = string_to_number(string_payload(rhs));
        return !std::isnan(converted) && bits_double(lhs) == converted ? 1 : 0;
    }
    if (is_number(rhs) && js_value_has_tag(lhs, JS_VALUE_STRING_TAG)) {
        const double converted = string_to_number(string_payload(lhs));
        return !std::isnan(converted) && bits_double(rhs) == converted ? 1 : 0;
    }
    return 0;
}

double nova_value_to_number(std::uint64_t value) {
    return value_to_number(value);
}

std::int64_t nova_value_is_nan(std::uint64_t value) {
    if (!is_number(value)) return 0;
    return std::isnan(bits_double(value)) ? 1 : 0;
}

std::uint64_t nova_value_to_number_boxed(std::uint64_t value) {
    return double_bits(value_to_number(value));
}

const char* nova_value_to_string_alloc(std::uint64_t value) {
    const std::string text = value_to_string(value);
    char* storage = static_cast<char*>(std::malloc(text.size() + 1));
    if (!storage) return "";
    std::memcpy(storage, text.c_str(), text.size() + 1);
    return storage;
}

std::uint64_t nova_value_to_primitive(std::uint64_t value, std::int32_t hint) {
    // Phase 2.1 stub: objects need valueOf/toString dispatch (added in 2.2
    // when the property storage and prototype chain land). For now, only
    // primitive values are handled — objects return themselves so the
    // existing arithmetic paths keep working for the common case.
    (void)hint;
    return value;
}

std::uint64_t nova_value_add(std::uint64_t lhs, std::uint64_t rhs) {
    if (js_value_has_tag(lhs, JS_VALUE_STRING_TAG) ||
        js_value_has_tag(rhs, JS_VALUE_STRING_TAG)) {
        const std::string result = value_to_string(lhs) + value_to_string(rhs);
        char* storage = static_cast<char*>(std::malloc(result.size() + 1));
        if (!storage) return JS_VALUE_UNDEFINED;
        std::memcpy(storage, result.c_str(), result.size() + 1);
        return pointer_value(JS_VALUE_STRING_TAG, storage);
    }
    return double_bits(value_to_number(lhs) + value_to_number(rhs));
}

std::uint64_t nova_value_binary_numeric(std::uint64_t lhs, std::uint64_t rhs,
                                        std::int64_t operation) {
    const double left = value_to_number(lhs);
    const double right = value_to_number(rhs);
    switch (operation) {
        case 0: return double_bits(left - right);
        case 1: return double_bits(left * right);
        case 2: return double_bits(left / right);
        case 3: return double_bits(std::fmod(left, right));
        case 4: return double_bits(std::pow(left, right));
        case 5: return double_bits(static_cast<double>(
            static_cast<std::int32_t>(left) & static_cast<std::int32_t>(right)));
        case 6: return double_bits(static_cast<double>(
            static_cast<std::int32_t>(left) | static_cast<std::int32_t>(right)));
        case 7: return double_bits(static_cast<double>(
            static_cast<std::int32_t>(left) ^ static_cast<std::int32_t>(right)));
        case 8: return double_bits(static_cast<double>(
            static_cast<std::int32_t>(left) << (static_cast<std::uint32_t>(right) & 31U)));
        case 9: return double_bits(static_cast<double>(
            static_cast<std::int32_t>(left) >> (static_cast<std::uint32_t>(right) & 31U)));
        case 10: return double_bits(static_cast<double>(
            static_cast<std::uint32_t>(left) >> (static_cast<std::uint32_t>(right) & 31U)));
        default: return JS_VALUE_CANONICAL_NAN;
    }
}

std::int64_t nova_value_compare(std::uint64_t lhs, std::uint64_t rhs,
                                std::int64_t operation) {
    if (js_value_has_tag(lhs, JS_VALUE_STRING_TAG) &&
        js_value_has_tag(rhs, JS_VALUE_STRING_TAG)) {
        const int comparison = std::strcmp(
            string_payload(lhs) ? string_payload(lhs) : "",
            string_payload(rhs) ? string_payload(rhs) : "");
        if (operation == 0) return comparison < 0;
        if (operation == 1) return comparison <= 0;
        if (operation == 2) return comparison > 0;
        return comparison >= 0;
    }
    const double left = value_to_number(lhs);
    const double right = value_to_number(rhs);
    if (std::isnan(left) || std::isnan(right)) return 0;
    if (operation == 0) return left < right;
    if (operation == 1) return left <= right;
    if (operation == 2) return left > right;
    return left >= right;
}

std::uint64_t nova_value_unary_numeric(std::uint64_t value,
                                       std::int64_t operation) {
    const double number = value_to_number(value);
    if (operation == 0) return double_bits(number);
    if (operation == 1) return double_bits(-number);
    return double_bits(static_cast<double>(~static_cast<std::int32_t>(number)));
}

void nova_console_log_value(std::uint64_t value) {
    if (value == JS_VALUE_UNDEFINED) std::printf("undefined");
    else if (value == JS_VALUE_NULL) std::printf("null");
    else if (value == JS_VALUE_FALSE) std::printf("false");
    else if (value == JS_VALUE_TRUE) std::printf("true");
    else if (js_value_has_tag(value, JS_VALUE_STRING_TAG)) {
        const char* text = string_payload(value);
        std::printf("%s", text ? text : "");
    } else if (js_value_has_tag(value, JS_VALUE_OBJECT_TAG)) {
        std::printf("[object Object]");
    } else {
        std::printf("%g", bits_double(value));
    }
    std::fflush(stdout);
}

// Returns the "length" of a JSValue-typed value, used by HIR when .length is
// accessed on a value whose static type is not known (e.g. an unannotated
// arrow-function parameter). Dispatches on the value's tag:
//   - String: strlen of the payload
//   - Object (ValueArray metadata or runtime Array): the length field at
//     offset 24 of the metadata struct
//   - Number: coerces to integer
//   - Undefined/Null: 0
std::int64_t nova_value_length(std::uint64_t value) {
    if (value == JS_VALUE_UNDEFINED || value == JS_VALUE_NULL) return 0;
    if (js_value_has_tag(value, JS_VALUE_STRING_TAG)) {
        const char* s = string_payload(value);
        return s ? static_cast<std::int64_t>(std::strlen(s)) : 0;
    }
    if (js_value_has_tag(value, JS_VALUE_OBJECT_TAG)) {
        // Treat payload as a ValueArray metadata pointer and read the
        // length field at offset 24 (after the 24-byte ObjectHeader).
        void* payload = reinterpret_cast<void*>(
            static_cast<std::uintptr_t>(value & JS_VALUE_PAYLOAD_MASK));
        if (!payload) return 0;
        const char* bytes = static_cast<const char*>(payload);
        return *reinterpret_cast<const std::int64_t*>(bytes + 24);
    }
    // Numeric: truncate to int.
    return static_cast<std::int64_t>(bits_double(value));
}

} // extern "C"
