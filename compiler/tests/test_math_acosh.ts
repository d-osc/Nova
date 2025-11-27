function main(): number {
    // Math.acosh() returns the inverse hyperbolic cosine of a number
    // acosh(x) = ln(x + sqrt(x^2 - 1))
    // Only defined for x >= 1
    // For integer type system, returns integer approximation
    // acosh(1) = 0 (always)
    // acosh(2) ≈ 1.317 → 1 as integer
    // acosh(3) ≈ 1.763 → 1 as integer
    // acosh(4) ≈ 2.063 → 2 as integer
    // Test basic functionality
    let a = Math.acosh(1);     // 0
    let b = Math.acosh(2);     // 1 (truncated from 1.317)
    let c = Math.acosh(3);     // 1 (truncated from 1.763)
    let d = Math.acosh(4);     // 2 (truncated from 2.063)
    // Result: 0 + 1 + 1 + 2 = 4
    return a + b + c + d;
}
