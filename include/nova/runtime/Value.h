#pragma once

#include <cstdint>

namespace nova::runtime {

using JSValue = std::uint64_t;

inline constexpr JSValue JS_VALUE_TAG_MASK = 0xffff000000000000ULL;
inline constexpr JSValue JS_VALUE_PAYLOAD_MASK = 0x0000ffffffffffffULL;
inline constexpr JSValue JS_VALUE_CANONICAL_NAN = 0x7ff8000000000000ULL;
inline constexpr JSValue JS_VALUE_UNDEFINED = 0x7ff9000000000000ULL;
inline constexpr JSValue JS_VALUE_NULL = 0x7ffa000000000000ULL;
inline constexpr JSValue JS_VALUE_FALSE = 0x7ffb000000000000ULL;
inline constexpr JSValue JS_VALUE_TRUE = 0x7ffc000000000000ULL;
inline constexpr JSValue JS_VALUE_STRING_TAG = 0x7ffd000000000000ULL;
inline constexpr JSValue JS_VALUE_OBJECT_TAG = 0x7ffe000000000000ULL;

inline constexpr bool js_value_has_tag(JSValue value, JSValue tag) {
    return (value & JS_VALUE_TAG_MASK) == tag;
}

} // namespace nova::runtime

extern "C" {
std::uint64_t nova_value_from_i64(std::int64_t value);
std::uint64_t nova_value_from_f64(double value);
std::uint64_t nova_value_from_bool(std::int64_t value);
std::uint64_t nova_value_from_string(const char* value);
std::uint64_t nova_value_from_object(void* value);
void* nova_value_to_object(std::uint64_t value);
const char* nova_value_to_string_ptr(std::uint64_t value);
std::int64_t nova_value_to_boolean(std::uint64_t value);
std::int64_t nova_value_is_nullish(std::uint64_t value);
std::int64_t nova_value_is_undefined(std::uint64_t value);
std::int64_t nova_value_strict_equal(std::uint64_t lhs, std::uint64_t rhs);
std::int64_t nova_value_abstract_equal(std::uint64_t lhs, std::uint64_t rhs);
double nova_value_to_number(std::uint64_t value);
std::int64_t nova_value_is_nan(std::uint64_t value);
std::uint64_t nova_value_to_number_boxed(std::uint64_t value);
const char* nova_value_to_string_alloc(std::uint64_t value);
std::uint64_t nova_value_to_primitive(std::uint64_t value, std::int32_t hint);
std::uint64_t nova_value_add(std::uint64_t lhs, std::uint64_t rhs);
std::uint64_t nova_value_binary_numeric(std::uint64_t lhs, std::uint64_t rhs,
                                        std::int64_t operation);
std::int64_t nova_value_compare(std::uint64_t lhs, std::uint64_t rhs,
                                std::int64_t operation);
std::uint64_t nova_value_unary_numeric(std::uint64_t value,
                                       std::int64_t operation);
void nova_console_log_value(std::uint64_t value);
std::int64_t nova_value_length(std::uint64_t value);
}
