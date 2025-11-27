function main(): number {
    // Math.expm1() returns e^x - 1
    // Useful for maintaining precision when x is near 0
    // For integer type system, returns integer approximation
    // expm1(0) = e^0 - 1 = 1 - 1 = 0
    // expm1(1) = e^1 - 1 ≈ 2.718 - 1 = 1.718 → 1 as integer
    // expm1(2) = e^2 - 1 ≈ 7.389 - 1 = 6.389 → 6 as integer
    // expm1(3) = e^3 - 1 ≈ 20.086 - 1 = 19.086 → 19 as integer

    // Test basic functionality
    let a = Math.expm1(0);     // 0
    let b = Math.expm1(1);     // 1 (truncated from 1.718)
    let c = Math.expm1(2);     // 6 (truncated from 6.389)
    let d = Math.expm1(3);     // 19 (truncated from 19.086)

    // Result: 0 + 1 + 6 + 19 = 26
    return a + b + c + d;
}
