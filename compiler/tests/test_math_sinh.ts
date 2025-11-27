function main(): number {
    // Math.sinh() returns the hyperbolic sine of a number
    // sinh(x) = (e^x - e^-x) / 2
    // For integer type system, returns integer approximation
    // sinh(0) = 0 (always)
    // sinh(1) ≈ 1.175 → 1 as integer
    // sinh(2) ≈ 3.627 → 3 as integer
    // Test basic functionality
    let a = Math.sinh(0);      // 0
    let b = Math.sinh(1);      // 1 (truncated from 1.175)
    let c = Math.sinh(2);      // 3 (truncated from 3.627)
    // Result: 0 + 1 + 3 = 4
    return a + b + c;
}
