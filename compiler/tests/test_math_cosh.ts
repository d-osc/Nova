function main(): number {
    // Math.cosh() returns the hyperbolic cosine of a number
    // cosh(x) = (e^x + e^-x) / 2
    // For integer type system, returns integer approximation
    // cosh(0) = 1 (always)
    // cosh(1) ≈ 1.543 → 1 as integer
    // cosh(2) ≈ 3.762 → 3 as integer
    // Test basic functionality
    let a = Math.cosh(0);      // 1
    let b = Math.cosh(1);      // 1 (truncated from 1.543)
    let c = Math.cosh(2);      // 3 (truncated from 3.762)
    // Result: 1 + 1 + 3 = 5
    return a + b + c;
}
