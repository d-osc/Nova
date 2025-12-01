// Nova Runtime - BigInt Implementation (ES2020)
// Implements arbitrary-precision integers

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

extern "C" {

// ============================================================================
// BigInt Structure - Arbitrary Precision Integer
// Uses a vector of 32-bit limbs for arbitrary precision
// ============================================================================
struct NovaBigInt {
    std::vector<uint32_t> limbs;  // Little-endian limbs (least significant first)
    bool negative;                 // Sign flag

    NovaBigInt() : negative(false) {
        limbs.push_back(0);
    }

    NovaBigInt(int64_t value) : negative(value < 0) {
        uint64_t absValue = negative ? static_cast<uint64_t>(-value) : static_cast<uint64_t>(value);
        if (absValue == 0) {
            limbs.push_back(0);
        } else {
            while (absValue > 0) {
                limbs.push_back(static_cast<uint32_t>(absValue & 0xFFFFFFFF));
                absValue >>= 32;
            }
        }
    }

    bool isZero() const {
        return limbs.size() == 1 && limbs[0] == 0;
    }

    void normalize() {
        while (limbs.size() > 1 && limbs.back() == 0) {
            limbs.pop_back();
        }
        if (isZero()) negative = false;
    }
};

// Forward declarations
void* nova_bigint_add(void* a, void* b);
void* nova_bigint_sub(void* a, void* b);
int nova_bigint_compare_abs(NovaBigInt* a, NovaBigInt* b);

// ============================================================================
// BigInt Creation
// ============================================================================

// Create BigInt from int64
void* nova_bigint_from_int64(int64_t value) {
    NovaBigInt* bigint = new NovaBigInt(value);
    return bigint;
}

// Create BigInt from string
void* nova_bigint_from_string(const char* str) {
    if (!str) return nova_bigint_from_int64(0);

    NovaBigInt* result = new NovaBigInt();
    result->limbs.clear();
    result->limbs.push_back(0);
    result->negative = false;

    const char* p = str;

    // Skip whitespace
    while (*p == ' ' || *p == '\t') p++;

    // Check sign
    if (*p == '-') {
        result->negative = true;
        p++;
    } else if (*p == '+') {
        p++;
    }

    // Parse digits
    while (*p >= '0' && *p <= '9') {
        // Multiply result by 10
        uint64_t carry = 0;
        for (size_t i = 0; i < result->limbs.size(); i++) {
            uint64_t prod = static_cast<uint64_t>(result->limbs[i]) * 10 + carry;
            result->limbs[i] = static_cast<uint32_t>(prod & 0xFFFFFFFF);
            carry = prod >> 32;
        }
        if (carry > 0) {
            result->limbs.push_back(static_cast<uint32_t>(carry));
        }

        // Add digit
        uint32_t digit = *p - '0';
        carry = digit;
        for (size_t i = 0; i < result->limbs.size() && carry > 0; i++) {
            uint64_t sum = static_cast<uint64_t>(result->limbs[i]) + carry;
            result->limbs[i] = static_cast<uint32_t>(sum & 0xFFFFFFFF);
            carry = sum >> 32;
        }
        if (carry > 0) {
            result->limbs.push_back(static_cast<uint32_t>(carry));
        }

        p++;
    }

    result->normalize();
    return result;
}

// BigInt(value) constructor - from number
void* nova_bigint_create(int64_t value) {
    return nova_bigint_from_int64(value);
}

// BigInt(string) constructor - from string
void* nova_bigint_create_from_string(const char* str) {
    return nova_bigint_from_string(str);
}

// Clone a BigInt
void* nova_bigint_clone(void* ptr) {
    if (!ptr) return nova_bigint_from_int64(0);
    NovaBigInt* src = static_cast<NovaBigInt*>(ptr);
    NovaBigInt* result = new NovaBigInt();
    result->limbs = src->limbs;
    result->negative = src->negative;
    return result;
}

// Free BigInt
void nova_bigint_free(void* ptr) {
    if (ptr) {
        delete static_cast<NovaBigInt*>(ptr);
    }
}

// ============================================================================
// BigInt Conversion
// ============================================================================

// Convert to string
const char* nova_bigint_toString(void* ptr, int64_t radix) {
    if (!ptr) return strdup("0");
    NovaBigInt* bigint = static_cast<NovaBigInt*>(ptr);

    if (radix < 2 || radix > 36) radix = 10;

    if (bigint->isZero()) {
        return strdup("0");
    }

    // Make a copy for division
    std::vector<uint32_t> temp = bigint->limbs;
    std::string digits;

    const char* digitChars = "0123456789abcdefghijklmnopqrstuvwxyz";

    while (!(temp.size() == 1 && temp[0] == 0)) {
        // Divide by radix
        uint64_t remainder = 0;
        for (int i = temp.size() - 1; i >= 0; i--) {
            uint64_t current = (remainder << 32) | temp[i];
            temp[i] = static_cast<uint32_t>(current / radix);
            remainder = current % radix;
        }

        // Remove leading zeros
        while (temp.size() > 1 && temp.back() == 0) {
            temp.pop_back();
        }

        digits += digitChars[remainder];
    }

    // Reverse and add sign
    std::reverse(digits.begin(), digits.end());
    if (bigint->negative) {
        digits = "-" + digits;
    }

    return strdup(digits.c_str());
}

// Convert to int64 (may lose precision)
int64_t nova_bigint_toInt64(void* ptr) {
    if (!ptr) return 0;
    NovaBigInt* bigint = static_cast<NovaBigInt*>(ptr);

    uint64_t result = 0;
    for (size_t i = 0; i < bigint->limbs.size() && i < 2; i++) {
        result |= static_cast<uint64_t>(bigint->limbs[i]) << (i * 32);
    }

    return bigint->negative ? -static_cast<int64_t>(result) : static_cast<int64_t>(result);
}

// valueOf - returns the primitive value (as int64 for simplicity)
int64_t nova_bigint_valueOf(void* ptr) {
    return nova_bigint_toInt64(ptr);
}

// ============================================================================
// BigInt Comparison
// ============================================================================

// Compare absolute values: returns -1 if |a| < |b|, 0 if equal, 1 if |a| > |b|
int nova_bigint_compare_abs(NovaBigInt* a, NovaBigInt* b) {
    if (a->limbs.size() != b->limbs.size()) {
        return a->limbs.size() < b->limbs.size() ? -1 : 1;
    }
    for (int i = a->limbs.size() - 1; i >= 0; i--) {
        if (a->limbs[i] != b->limbs[i]) {
            return a->limbs[i] < b->limbs[i] ? -1 : 1;
        }
    }
    return 0;
}

// Compare: returns -1 if a < b, 0 if equal, 1 if a > b
int64_t nova_bigint_compare(void* aPtr, void* bPtr) {
    if (!aPtr && !bPtr) return 0;
    if (!aPtr) return -1;
    if (!bPtr) return 1;

    NovaBigInt* a = static_cast<NovaBigInt*>(aPtr);
    NovaBigInt* b = static_cast<NovaBigInt*>(bPtr);

    if (a->negative != b->negative) {
        return a->negative ? -1 : 1;
    }

    int cmp = nova_bigint_compare_abs(a, b);
    return a->negative ? -cmp : cmp;
}

// Equality check
int64_t nova_bigint_equals(void* aPtr, void* bPtr) {
    return nova_bigint_compare(aPtr, bPtr) == 0 ? 1 : 0;
}

// Less than
int64_t nova_bigint_lt(void* aPtr, void* bPtr) {
    return nova_bigint_compare(aPtr, bPtr) < 0 ? 1 : 0;
}

// Less than or equal
int64_t nova_bigint_le(void* aPtr, void* bPtr) {
    return nova_bigint_compare(aPtr, bPtr) <= 0 ? 1 : 0;
}

// Greater than
int64_t nova_bigint_gt(void* aPtr, void* bPtr) {
    return nova_bigint_compare(aPtr, bPtr) > 0 ? 1 : 0;
}

// Greater than or equal
int64_t nova_bigint_ge(void* aPtr, void* bPtr) {
    return nova_bigint_compare(aPtr, bPtr) >= 0 ? 1 : 0;
}

// ============================================================================
// BigInt Arithmetic - Addition
// ============================================================================

// Add absolute values
void* nova_bigint_add_abs(NovaBigInt* a, NovaBigInt* b) {
    NovaBigInt* result = new NovaBigInt();
    result->limbs.clear();

    size_t maxLen = std::max(a->limbs.size(), b->limbs.size());
    uint64_t carry = 0;

    for (size_t i = 0; i < maxLen || carry > 0; i++) {
        uint64_t sum = carry;
        if (i < a->limbs.size()) sum += a->limbs[i];
        if (i < b->limbs.size()) sum += b->limbs[i];

        result->limbs.push_back(static_cast<uint32_t>(sum & 0xFFFFFFFF));
        carry = sum >> 32;
    }

    result->normalize();
    return result;
}

// Subtract absolute values (assumes |a| >= |b|)
void* nova_bigint_sub_abs(NovaBigInt* a, NovaBigInt* b) {
    NovaBigInt* result = new NovaBigInt();
    result->limbs.clear();

    int64_t borrow = 0;

    for (size_t i = 0; i < a->limbs.size(); i++) {
        int64_t diff = static_cast<int64_t>(a->limbs[i]) - borrow;
        if (i < b->limbs.size()) {
            diff -= b->limbs[i];
        }

        if (diff < 0) {
            diff += (1LL << 32);
            borrow = 1;
        } else {
            borrow = 0;
        }

        result->limbs.push_back(static_cast<uint32_t>(diff));
    }

    result->normalize();
    return result;
}

// BigInt addition
void* nova_bigint_add(void* aPtr, void* bPtr) {
    if (!aPtr) return nova_bigint_clone(bPtr);
    if (!bPtr) return nova_bigint_clone(aPtr);

    NovaBigInt* a = static_cast<NovaBigInt*>(aPtr);
    NovaBigInt* b = static_cast<NovaBigInt*>(bPtr);

    NovaBigInt* result;

    if (a->negative == b->negative) {
        // Same sign: add absolute values
        result = static_cast<NovaBigInt*>(nova_bigint_add_abs(a, b));
        result->negative = a->negative;
    } else {
        // Different signs: subtract absolute values
        int cmp = nova_bigint_compare_abs(a, b);
        if (cmp >= 0) {
            result = static_cast<NovaBigInt*>(nova_bigint_sub_abs(a, b));
            result->negative = a->negative;
        } else {
            result = static_cast<NovaBigInt*>(nova_bigint_sub_abs(b, a));
            result->negative = b->negative;
        }
    }

    result->normalize();
    return result;
}

// BigInt subtraction
void* nova_bigint_sub(void* aPtr, void* bPtr) {
    if (!bPtr) return nova_bigint_clone(aPtr);

    // a - b = a + (-b)
    NovaBigInt* b = static_cast<NovaBigInt*>(bPtr);
    NovaBigInt* negB = static_cast<NovaBigInt*>(nova_bigint_clone(bPtr));
    negB->negative = !negB->negative;
    if (negB->isZero()) negB->negative = false;

    void* result = nova_bigint_add(aPtr, negB);
    delete negB;
    return result;
}

// BigInt negation
void* nova_bigint_negate(void* ptr) {
    if (!ptr) return nova_bigint_from_int64(0);
    NovaBigInt* result = static_cast<NovaBigInt*>(nova_bigint_clone(ptr));
    result->negative = !result->negative;
    if (result->isZero()) result->negative = false;
    return result;
}

// ============================================================================
// BigInt Arithmetic - Multiplication
// ============================================================================

void* nova_bigint_mul(void* aPtr, void* bPtr) {
    if (!aPtr || !bPtr) return nova_bigint_from_int64(0);

    NovaBigInt* a = static_cast<NovaBigInt*>(aPtr);
    NovaBigInt* b = static_cast<NovaBigInt*>(bPtr);

    if (a->isZero() || b->isZero()) {
        return nova_bigint_from_int64(0);
    }

    NovaBigInt* result = new NovaBigInt();
    result->limbs.assign(a->limbs.size() + b->limbs.size(), 0);
    result->negative = a->negative != b->negative;

    for (size_t i = 0; i < a->limbs.size(); i++) {
        uint64_t carry = 0;
        for (size_t j = 0; j < b->limbs.size() || carry > 0; j++) {
            uint64_t prod = result->limbs[i + j] + carry;
            if (j < b->limbs.size()) {
                prod += static_cast<uint64_t>(a->limbs[i]) * b->limbs[j];
            }
            result->limbs[i + j] = static_cast<uint32_t>(prod & 0xFFFFFFFF);
            carry = prod >> 32;
        }
    }

    result->normalize();
    return result;
}

// ============================================================================
// BigInt Arithmetic - Division and Modulo
// ============================================================================

// Simple long division
void nova_bigint_divmod(void* aPtr, void* bPtr, void** quotient, void** remainder) {
    NovaBigInt* a = static_cast<NovaBigInt*>(aPtr);
    NovaBigInt* b = static_cast<NovaBigInt*>(bPtr);

    if (!b || b->isZero()) {
        // Division by zero - return 0 for both
        *quotient = nova_bigint_from_int64(0);
        *remainder = nova_bigint_from_int64(0);
        return;
    }

    if (!a || a->isZero()) {
        *quotient = nova_bigint_from_int64(0);
        *remainder = nova_bigint_from_int64(0);
        return;
    }

    int cmp = nova_bigint_compare_abs(a, b);
    if (cmp < 0) {
        // |a| < |b|, quotient is 0, remainder is a
        *quotient = nova_bigint_from_int64(0);
        *remainder = nova_bigint_clone(aPtr);
        return;
    }

    if (cmp == 0) {
        // |a| == |b|
        *quotient = nova_bigint_from_int64(a->negative == b->negative ? 1 : -1);
        *remainder = nova_bigint_from_int64(0);
        return;
    }

    // Long division algorithm
    NovaBigInt* q = new NovaBigInt();
    q->limbs.assign(a->limbs.size(), 0);

    NovaBigInt* r = new NovaBigInt();
    r->limbs.clear();
    r->limbs.push_back(0);

    // Process bits from most significant to least significant
    for (int i = a->limbs.size() * 32 - 1; i >= 0; i--) {
        // Shift remainder left by 1 bit
        uint32_t carry = 0;
        for (size_t j = 0; j < r->limbs.size(); j++) {
            uint32_t newCarry = r->limbs[j] >> 31;
            r->limbs[j] = (r->limbs[j] << 1) | carry;
            carry = newCarry;
        }
        if (carry) {
            r->limbs.push_back(carry);
        }

        // Add bit from dividend
        int limbIdx = i / 32;
        int bitIdx = i % 32;
        if ((a->limbs[limbIdx] >> bitIdx) & 1) {
            uint64_t sum = r->limbs[0] + 1;
            r->limbs[0] = static_cast<uint32_t>(sum & 0xFFFFFFFF);
            uint64_t c = sum >> 32;
            for (size_t j = 1; j < r->limbs.size() && c > 0; j++) {
                sum = r->limbs[j] + c;
                r->limbs[j] = static_cast<uint32_t>(sum & 0xFFFFFFFF);
                c = sum >> 32;
            }
            if (c > 0) {
                r->limbs.push_back(static_cast<uint32_t>(c));
            }
        }

        // Compare r with b
        r->normalize();
        if (nova_bigint_compare_abs(r, b) >= 0) {
            // Subtract b from r
            NovaBigInt* newR = static_cast<NovaBigInt*>(nova_bigint_sub_abs(r, b));
            delete r;
            r = newR;

            // Set quotient bit
            q->limbs[limbIdx] |= (1U << bitIdx);
        }
    }

    q->negative = a->negative != b->negative;
    r->negative = a->negative;

    q->normalize();
    r->normalize();

    *quotient = q;
    *remainder = r;
}

void* nova_bigint_div(void* aPtr, void* bPtr) {
    void* quotient;
    void* remainder;
    nova_bigint_divmod(aPtr, bPtr, &quotient, &remainder);
    nova_bigint_free(remainder);
    return quotient;
}

void* nova_bigint_mod(void* aPtr, void* bPtr) {
    void* quotient;
    void* remainder;
    nova_bigint_divmod(aPtr, bPtr, &quotient, &remainder);
    nova_bigint_free(quotient);
    return remainder;
}

// ============================================================================
// BigInt Arithmetic - Exponentiation
// ============================================================================

void* nova_bigint_pow(void* basePtr, void* expPtr) {
    if (!basePtr || !expPtr) return nova_bigint_from_int64(0);

    NovaBigInt* exp = static_cast<NovaBigInt*>(expPtr);

    if (exp->negative) {
        // Negative exponents not supported for BigInt
        return nova_bigint_from_int64(0);
    }

    if (exp->isZero()) {
        return nova_bigint_from_int64(1);
    }

    // Binary exponentiation
    void* result = nova_bigint_from_int64(1);
    void* base = nova_bigint_clone(basePtr);
    void* e = nova_bigint_clone(expPtr);
    void* two = nova_bigint_from_int64(2);

    NovaBigInt* ePtr = static_cast<NovaBigInt*>(e);

    while (!ePtr->isZero()) {
        // Check if exp is odd
        if (ePtr->limbs[0] & 1) {
            void* newResult = nova_bigint_mul(result, base);
            nova_bigint_free(result);
            result = newResult;
        }

        // base = base * base
        void* newBase = nova_bigint_mul(base, base);
        nova_bigint_free(base);
        base = newBase;

        // exp = exp / 2
        void* newE = nova_bigint_div(e, two);
        nova_bigint_free(e);
        e = newE;
        ePtr = static_cast<NovaBigInt*>(e);
    }

    nova_bigint_free(base);
    nova_bigint_free(e);
    nova_bigint_free(two);

    return result;
}

// ============================================================================
// BigInt Bitwise Operations
// ============================================================================

void* nova_bigint_and(void* aPtr, void* bPtr) {
    if (!aPtr || !bPtr) return nova_bigint_from_int64(0);

    NovaBigInt* a = static_cast<NovaBigInt*>(aPtr);
    NovaBigInt* b = static_cast<NovaBigInt*>(bPtr);

    // For simplicity, only handle non-negative numbers
    if (a->negative || b->negative) {
        // Two's complement would be needed for negative numbers
        // For now, return 0 for negative operands
        return nova_bigint_from_int64(0);
    }

    NovaBigInt* result = new NovaBigInt();
    result->limbs.clear();

    size_t minLen = std::min(a->limbs.size(), b->limbs.size());
    for (size_t i = 0; i < minLen; i++) {
        result->limbs.push_back(a->limbs[i] & b->limbs[i]);
    }

    if (result->limbs.empty()) {
        result->limbs.push_back(0);
    }

    result->normalize();
    return result;
}

void* nova_bigint_or(void* aPtr, void* bPtr) {
    if (!aPtr) return nova_bigint_clone(bPtr);
    if (!bPtr) return nova_bigint_clone(aPtr);

    NovaBigInt* a = static_cast<NovaBigInt*>(aPtr);
    NovaBigInt* b = static_cast<NovaBigInt*>(bPtr);

    if (a->negative || b->negative) {
        return nova_bigint_from_int64(0);
    }

    NovaBigInt* result = new NovaBigInt();
    result->limbs.clear();

    size_t maxLen = std::max(a->limbs.size(), b->limbs.size());
    for (size_t i = 0; i < maxLen; i++) {
        uint32_t av = i < a->limbs.size() ? a->limbs[i] : 0;
        uint32_t bv = i < b->limbs.size() ? b->limbs[i] : 0;
        result->limbs.push_back(av | bv);
    }

    result->normalize();
    return result;
}

void* nova_bigint_xor(void* aPtr, void* bPtr) {
    if (!aPtr) return nova_bigint_clone(bPtr);
    if (!bPtr) return nova_bigint_clone(aPtr);

    NovaBigInt* a = static_cast<NovaBigInt*>(aPtr);
    NovaBigInt* b = static_cast<NovaBigInt*>(bPtr);

    if (a->negative || b->negative) {
        return nova_bigint_from_int64(0);
    }

    NovaBigInt* result = new NovaBigInt();
    result->limbs.clear();

    size_t maxLen = std::max(a->limbs.size(), b->limbs.size());
    for (size_t i = 0; i < maxLen; i++) {
        uint32_t av = i < a->limbs.size() ? a->limbs[i] : 0;
        uint32_t bv = i < b->limbs.size() ? b->limbs[i] : 0;
        result->limbs.push_back(av ^ bv);
    }

    result->normalize();
    return result;
}

void* nova_bigint_not(void* ptr) {
    // Bitwise NOT for BigInt is complex due to infinite precision
    // For now, implement as -(n + 1) which is equivalent for two's complement
    void* one = nova_bigint_from_int64(1);
    void* sum = nova_bigint_add(ptr, one);
    void* result = nova_bigint_negate(sum);
    nova_bigint_free(one);
    nova_bigint_free(sum);
    return result;
}

void* nova_bigint_shl(void* aPtr, int64_t shift) {
    if (!aPtr || shift == 0) return nova_bigint_clone(aPtr);
    if (shift < 0) return nova_bigint_from_int64(0);  // Right shift would need different impl

    NovaBigInt* a = static_cast<NovaBigInt*>(aPtr);
    NovaBigInt* result = new NovaBigInt();
    result->negative = a->negative;

    int limbShift = shift / 32;
    int bitShift = shift % 32;

    result->limbs.assign(limbShift, 0);  // Add zero limbs for limb shift

    uint32_t carry = 0;
    for (size_t i = 0; i < a->limbs.size(); i++) {
        uint64_t val = (static_cast<uint64_t>(a->limbs[i]) << bitShift) | carry;
        result->limbs.push_back(static_cast<uint32_t>(val & 0xFFFFFFFF));
        carry = val >> 32;
    }
    if (carry > 0) {
        result->limbs.push_back(carry);
    }

    result->normalize();
    return result;
}

void* nova_bigint_shr(void* aPtr, int64_t shift) {
    if (!aPtr || shift == 0) return nova_bigint_clone(aPtr);
    if (shift < 0) return nova_bigint_from_int64(0);

    NovaBigInt* a = static_cast<NovaBigInt*>(aPtr);

    int limbShift = shift / 32;
    int bitShift = shift % 32;

    if (limbShift >= static_cast<int>(a->limbs.size())) {
        return nova_bigint_from_int64(a->negative ? -1 : 0);
    }

    NovaBigInt* result = new NovaBigInt();
    result->negative = a->negative;
    result->limbs.clear();

    for (size_t i = limbShift; i < a->limbs.size(); i++) {
        uint32_t val = a->limbs[i] >> bitShift;
        if (i + 1 < a->limbs.size() && bitShift > 0) {
            val |= a->limbs[i + 1] << (32 - bitShift);
        }
        result->limbs.push_back(val);
    }

    if (result->limbs.empty()) {
        result->limbs.push_back(0);
    }

    result->normalize();
    return result;
}

// ============================================================================
// BigInt Static Methods
// ============================================================================

// BigInt.asIntN(bits, bigint) - Wraps to signed integer of specified bits
void* nova_bigint_asIntN(int64_t bits, void* ptr) {
    if (!ptr || bits <= 0) return nova_bigint_from_int64(0);

    // Get the value modulo 2^bits
    void* twoPowBits = nova_bigint_shl(nova_bigint_from_int64(1), bits);
    void* remainder;
    void* quotient;
    nova_bigint_divmod(ptr, twoPowBits, &quotient, &remainder);
    nova_bigint_free(quotient);

    NovaBigInt* rem = static_cast<NovaBigInt*>(remainder);

    // Check if we need to convert to negative (if high bit is set)
    void* twoPowBitsMinus1 = nova_bigint_shl(nova_bigint_from_int64(1), bits - 1);
    if (nova_bigint_compare(remainder, twoPowBitsMinus1) >= 0) {
        // Subtract 2^bits to make it negative
        void* result = nova_bigint_sub(remainder, twoPowBits);
        nova_bigint_free(remainder);
        nova_bigint_free(twoPowBits);
        nova_bigint_free(twoPowBitsMinus1);
        return result;
    }

    nova_bigint_free(twoPowBits);
    nova_bigint_free(twoPowBitsMinus1);
    return remainder;
}

// BigInt.asUintN(bits, bigint) - Wraps to unsigned integer of specified bits
void* nova_bigint_asUintN(int64_t bits, void* ptr) {
    if (!ptr || bits <= 0) return nova_bigint_from_int64(0);

    // Get the value modulo 2^bits
    void* twoPowBits = nova_bigint_shl(nova_bigint_from_int64(1), bits);
    void* remainder;
    void* quotient;
    nova_bigint_divmod(ptr, twoPowBits, &quotient, &remainder);

    nova_bigint_free(quotient);
    nova_bigint_free(twoPowBits);

    // Handle negative remainders
    NovaBigInt* rem = static_cast<NovaBigInt*>(remainder);
    if (rem->negative) {
        twoPowBits = nova_bigint_shl(nova_bigint_from_int64(1), bits);
        void* result = nova_bigint_add(remainder, twoPowBits);
        nova_bigint_free(remainder);
        nova_bigint_free(twoPowBits);
        return result;
    }

    return remainder;
}

// ============================================================================
// BigInt Unary Operations
// ============================================================================

// Unary plus (just returns clone)
void* nova_bigint_unary_plus(void* ptr) {
    return nova_bigint_clone(ptr);
}

// Unary minus (negation)
void* nova_bigint_unary_minus(void* ptr) {
    return nova_bigint_negate(ptr);
}

// Increment
void* nova_bigint_inc(void* ptr) {
    void* one = nova_bigint_from_int64(1);
    void* result = nova_bigint_add(ptr, one);
    nova_bigint_free(one);
    return result;
}

// Decrement
void* nova_bigint_dec(void* ptr) {
    void* one = nova_bigint_from_int64(1);
    void* result = nova_bigint_sub(ptr, one);
    nova_bigint_free(one);
    return result;
}

} // extern "C"
