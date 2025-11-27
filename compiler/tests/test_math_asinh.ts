function main(): number {
    // Math.asinh() returns the inverse hyperbolic sine of a number
    // asinh(x) = ln(x + sqrt(x^2 + 1))
    // For integer type system, returns integer approximation
    // asinh(0) = 0 (always)
    // asinh(1) ≈ 0.881 → 0 as integer
    // asinh(2) ≈ 1.444 → 1 as integer
    // asinh(3) ≈ 1.818 → 1 as integer
    // Test basic functionality
    let a = Math.asinh(0);     // 0
    let b = Math.asinh(1);     // 0 (truncated from 0.881)
    let c = Math.asinh(2);     // 1 (truncated from 1.444)
    let d = Math.asinh(3);     // 1 (truncated from 1.818)
    // Result: 0 + 0 + 1 + 1 = 2
    return a + b + c + d;
}
