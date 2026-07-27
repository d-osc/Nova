// Nova Math Runtime Support
// Provides runtime functions for Math object methods that need custom implementation

#include "nova/runtime/Runtime.h"
#include <cstdint>
#include <cmath>
#include <cstring>
#include <random>
#include <ctime>
#include <iostream>

namespace nova {
namespace runtime {

// Static random number generator for Math.random()
// Using Mersenne Twister for better quality random numbers than rand()
static std::mt19937_64 rng;
static bool rng_initialized = false;

// Initialize RNG with seed based on current time
static void init_rng() {
    if (!rng_initialized) {
        // Seed with current time for non-deterministic behavior
        rng.seed(static_cast<uint64_t>(std::time(nullptr)));
        rng_initialized = true;
    }
}

} // namespace runtime
} // namespace nova

// Extern "C" wrappers for Nova runtime
extern "C" {

// Math.random() - Returns a pseudo-random number between 0.0 and 1.0
// JavaScript spec: Returns a Number value with positive sign, greater than or equal to 0
// but less than 1, chosen randomly or pseudo randomly with approximately uniform distribution
double nova_random() {
    using namespace nova::runtime;

    // Initialize RNG on first call
    if (!rng_initialized) {
        init_rng();
    }

    // Generate random double between 0.0 and 1.0
    // Using uniform_real_distribution for proper range [0.0, 1.0)
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double random_value = dist(rng);

    return random_value;
}

// Math.ceil(x) - Smallest integer greater than or equal to x
int64_t nova_math_ceil(double x) {
    return static_cast<int64_t>(std::ceil(x));
}

// Math.floor(x) - Largest integer less than or equal to x
int64_t nova_math_floor(double x) {
    return static_cast<int64_t>(std::floor(x));
}

// Math.round(x) - Round to nearest integer; halfway cases round toward +Infinity
// Per JavaScript spec: Math.round(-3.5) = -3, Math.round(3.5) = 4
int64_t nova_math_round(double x) {
    return static_cast<int64_t>(std::floor(x + 0.5));
}

// Math.trunc(x) - Integer part of x (truncate toward 0)
int64_t nova_math_trunc(double x) {
    return static_cast<int64_t>(std::trunc(x));
}

} // extern "C"
