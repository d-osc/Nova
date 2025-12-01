function main(): number {
    // Math.asin() returns arcsine (inverse sine) of a number
    // Input must be between -1 and 1
    // Result is in radians, between -π/2 and π/2
    // For integer type system, returns integer approximation
    // asin(0) = 0 (always)
    // asin(1) ≈ 1.571 (π/2) → 1 as integer
    // Test basic functionality
    let a = Math.asin(0);      // 0
    let b = Math.asin(1);      // 1 (truncated from 1.571)
    // Result: 0 + 1 = 1
    return a + b;
}
