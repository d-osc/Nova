function main(): number {
    // Math.log() returns natural logarithm
    // For integer type system, returns integer approximation
    // log(1) = 0 (always)
    // log(e) ≈ 1  (e ≈ 2.718, as integer = 2, log(2) ≈ 0.693 → 0 or 1)
    // log(8) ≈ 2.079 → 2
    let a = Math.log(1);      // 0
    let b = Math.log(8);      // 2
    // Result: 0 + 2 = 2
    return a + b;
}
