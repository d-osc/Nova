function main(): number {
    // Math.exp() returns e^x (exponential function)
    // For integer type system, returns integer approximation
    // exp(0) = 1 (always)
    // exp(1) ≈ 2.718 → 2 as integer
    // exp(2) ≈ 7.389 → 7 as integer
    let a = Math.exp(0);      // 1
    let b = Math.exp(2);      // 7
    // Result: 1 + 7 = 8
    return a + b;
}
