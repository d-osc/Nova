#include "nova/runtime/Runtime.h"
#include <cstring>
#include <cstdio>
#include <cmath>

namespace nova {
namespace runtime {

// No internal helper functions needed for now

} // namespace runtime
} // namespace nova

// Extern "C" wrapper for Number instance methods
extern "C" {

// Number.prototype.toFixed(digits) - formats number with fixed decimal places
const char* nova_number_toFixed(double num, int64_t digits) {
    // Validate digits range (JavaScript spec: 0-100)
    if (digits < 0 || digits > 100) {
        // In JavaScript, this would throw RangeError
        // For now, clamp to valid range
        if (digits < 0) digits = 0;
        if (digits > 100) digits = 100;
    }

    // Handle special values
    if (std::isnan(num)) {
        char* result = new char[4];
        std::strcpy(result, "NaN");
        return result;
    }
    if (std::isinf(num)) {
        if (num > 0) {
            char* result = new char[9];
            std::strcpy(result, "Infinity");
            return result;
        } else {
            char* result = new char[10];
            std::strcpy(result, "-Infinity");
            return result;
        }
    }

    // Format the number with specified decimal places
    // Use snprintf with dynamic format string
    char format[32];
    std::snprintf(format, sizeof(format), "%%.%lldf", (long long)digits);

    // Calculate buffer size needed
    // Maximum: sign(1) + digits(308 max for double) + decimal point(1) + fraction digits(100 max) + null(1)
    int bufferSize = 512;
    char* buffer = new char[bufferSize];

    std::snprintf(buffer, bufferSize, format, num);

    return buffer;
}

// Number.prototype.toExponential(fractionDigits) - formats number in exponential notation
const char* nova_number_toExponential(double num, int64_t fractionDigits) {
    // Validate fractionDigits range (JavaScript spec: 0-100)
    if (fractionDigits < 0 || fractionDigits > 100) {
        // In JavaScript, this would throw RangeError
        // For now, clamp to valid range
        if (fractionDigits < 0) fractionDigits = 0;
        if (fractionDigits > 100) fractionDigits = 100;
    }

    // Handle special values
    if (std::isnan(num)) {
        char* result = new char[4];
        std::strcpy(result, "NaN");
        return result;
    }
    if (std::isinf(num)) {
        if (num > 0) {
            char* result = new char[9];
            std::strcpy(result, "Infinity");
            return result;
        } else {
            char* result = new char[10];
            std::strcpy(result, "-Infinity");
            return result;
        }
    }

    // Format the number in exponential notation
    // Use snprintf with %e format (exponential notation)
    char format[32];
    std::snprintf(format, sizeof(format), "%%.%lde", (long)fractionDigits);

    // Calculate buffer size needed
    // Format: [-]d.ddd...e[+/-]ddd
    // Maximum: sign(1) + digit(1) + decimal(1) + fraction(100) + 'e'(1) + exponent sign(1) + exponent(3) + null(1)
    int bufferSize = 256;
    char* buffer = new char[bufferSize];

    std::snprintf(buffer, bufferSize, format, num);

    return buffer;
}

} // extern "C"
