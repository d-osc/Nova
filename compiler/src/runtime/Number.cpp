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

// Number.prototype.toPrecision(precision) - formats number with specified precision
const char* nova_number_toPrecision(double num, int64_t precision) {
    // Validate precision range (JavaScript spec: 1-100)
    if (precision < 1 || precision > 100) {
        // In JavaScript, this would throw RangeError
        // For now, clamp to valid range
        if (precision < 1) precision = 1;
        if (precision > 100) precision = 100;
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

    // Format the number with specified precision (significant digits)
    // Use snprintf with %g format (chooses between %f and %e based on value)
    // The precision in %g is the number of significant digits
    char format[32];
    std::snprintf(format, sizeof(format), "%%.%ldg", (long)precision);

    // Calculate buffer size needed
    // Maximum: sign(1) + digits(100) + decimal(1) + 'e'(1) + exponent sign(1) + exponent(3) + null(1)
    int bufferSize = 256;
    char* buffer = new char[bufferSize];

    std::snprintf(buffer, bufferSize, format, num);

    return buffer;
}

// Number.prototype.toString(radix) - converts number to string with optional radix
const char* nova_number_toString(double num, int64_t radix) {
    // Validate radix range (JavaScript spec: 2-36, default 10)
    if (radix < 2 || radix > 36) {
        radix = 10;  // Default to base 10 if invalid
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

    // For base 10, use simple sprintf
    if (radix == 10) {
        char* buffer = new char[64];
        std::snprintf(buffer, 64, "%.0f", num);
        return buffer;
    }

    // For other bases, convert to integer first (JavaScript behavior)
    int64_t intNum = static_cast<int64_t>(num);

    // Handle negative numbers
    bool isNegative = intNum < 0;
    if (isNegative) {
        intNum = -intNum;
    }

    // Convert to specified radix
    char* buffer = new char[128];
    char* ptr = buffer + 127;
    *ptr = '\0';

    const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

    if (intNum == 0) {
        *--ptr = '0';
    } else {
        while (intNum > 0) {
            *--ptr = digits[intNum % radix];
            intNum /= radix;
        }
    }

    if (isNegative) {
        *--ptr = '-';
    }

    // Copy result to new buffer
    size_t len = buffer + 127 - ptr;
    char* result = new char[len + 1];
    std::strcpy(result, ptr);

    delete[] buffer;
    return result;
}

// Number.prototype.valueOf() - returns the primitive value
double nova_number_valueOf(double num) {
    // Simply return the number itself
    // In JavaScript, this unwraps a Number object to its primitive value
    // For primitive numbers (which we use), this is just an identity function
    return num;
}

} // extern "C"
