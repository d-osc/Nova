// Ultra-Optimized Fibonacci Implementation
// Provides multiple algorithms for maximum performance

#include "nova/runtime/Runtime.h"
#include <cstdint>
#include <array>

namespace nova {
namespace runtime {

// ==================== OPTIMIZATION 1: Memoization ====================
// Cache for Fibonacci results (supports up to fib(92) without overflow)
static std::array<int64_t, 93> fib_cache = {0};
static bool fib_cache_initialized = false;

// Initialize cache with iterative approach
static void init_fib_cache() {
    if (fib_cache_initialized) return;

    fib_cache[0] = 0;
    fib_cache[1] = 1;

    // Compute all values up to fib(92)
    for (int i = 2; i < 93; i++) {
        fib_cache[i] = fib_cache[i-1] + fib_cache[i-2];
    }

    fib_cache_initialized = true;
}

// OPTIMIZATION 2: Fast lookup - O(1) for cached values
int64_t fib_memoized(int64_t n) {
    if (n < 0) return 0;
    if (n >= 93) return fib_cache[92];  // Overflow protection

    if (!fib_cache_initialized) {
        init_fib_cache();
    }

    return fib_cache[n];
}

// ==================== OPTIMIZATION 3: Iterative (Space-Optimized) ====================
// Uses only O(1) space, no recursion overhead
int64_t fib_iterative(int64_t n) {
    if (n <= 1) return n;

    int64_t a = 0;
    int64_t b = 1;

    // Loop unrolling candidate - LLVM will optimize this
    for (int64_t i = 2; i <= n; i++) {
        int64_t temp = a + b;
        a = b;
        b = temp;
    }

    return b;
}

// ==================== OPTIMIZATION 4: Matrix Exponentiation ====================
// O(log n) complexity using fast matrix multiplication
struct Matrix2x2 {
    int64_t a, b, c, d;

    Matrix2x2(int64_t a_, int64_t b_, int64_t c_, int64_t d_)
        : a(a_), b(b_), c(c_), d(d_) {}

    Matrix2x2 multiply(const Matrix2x2& other) const {
        return Matrix2x2(
            a * other.a + b * other.c,
            a * other.b + b * other.d,
            c * other.a + d * other.c,
            c * other.b + d * other.d
        );
    }
};

Matrix2x2 matrix_power(Matrix2x2 base, int64_t exp) {
    if (exp == 1) return base;

    if (exp % 2 == 0) {
        Matrix2x2 half = matrix_power(base, exp / 2);
        return half.multiply(half);
    } else {
        return base.multiply(matrix_power(base, exp - 1));
    }
}

int64_t fib_matrix(int64_t n) {
    if (n <= 1) return n;

    Matrix2x2 base(1, 1, 1, 0);
    Matrix2x2 result = matrix_power(base, n);

    return result.c;  // F(n) is in the c position
}

// ==================== OPTIMIZATION 5: Binet's Formula ====================
// Closed-form formula using golden ratio
// Note: Limited precision for large n due to floating point
#include <cmath>

int64_t fib_binet(int64_t n) {
    if (n <= 1) return n;

    const double phi = (1.0 + sqrt(5.0)) / 2.0;      // Golden ratio
    const double psi = (1.0 - sqrt(5.0)) / 2.0;      // Conjugate
    const double sqrt5 = sqrt(5.0);

    // Binet's formula: F(n) = (phi^n - psi^n) / sqrt(5)
    double result = (pow(phi, n) - pow(psi, n)) / sqrt5;

    return static_cast<int64_t>(result + 0.5);  // Round to nearest
}

// ==================== OPTIMIZATION 6: Hybrid Approach ====================
// Chooses best algorithm based on input
int64_t fib_ultra(int64_t n) {
    // Fast path: Use memoization for n < 93
    if (n < 93) {
        return fib_memoized(n);
    }

    // For larger n, use matrix method (more accurate than Binet)
    return fib_matrix(n);
}

} // namespace runtime
} // namespace nova

// ==================== C API for Nova Runtime ====================
extern "C" {

// Ultra-fast Fibonacci using memoization
int64_t nova_fib_fast(int64_t n) {
    return nova::runtime::fib_memoized(n);
}

// Iterative Fibonacci (no memoization)
int64_t nova_fib_iterative(int64_t n) {
    return nova::runtime::fib_iterative(n);
}

// Matrix exponentiation Fibonacci (O(log n))
int64_t nova_fib_matrix(int64_t n) {
    return nova::runtime::fib_matrix(n);
}

// Binet's formula Fibonacci (O(1) but floating point)
int64_t nova_fib_binet(int64_t n) {
    return nova::runtime::fib_binet(n);
}

// Hybrid ultra-optimized Fibonacci
int64_t nova_fib_ultra(int64_t n) {
    return nova::runtime::fib_ultra(n);
}

// Initialize the Fibonacci cache at startup
void nova_fib_init() {
    nova::runtime::init_fib_cache();
}

} // extern "C"
