function main(): number {
    // Math.log1p() returns ln(1 + x)
    // Useful for maintaining precision when x is near 0
    // For integer type system, returns integer approximation
    // log1p(0) = ln(1 + 0) = ln(1) = 0
    // log1p(1) = ln(1 + 1) = ln(2) ≈ 0.693 → 0 as integer
    // log1p(2) = ln(1 + 2) = ln(3) ≈ 1.099 → 1 as integer
    // log1p(6) = ln(1 + 6) = ln(7) ≈ 1.946 → 1 as integer
    // log1p(9) = ln(1 + 9) = ln(10) ≈ 2.303 → 2 as integer

    // Test basic functionality
    let a = Math.log1p(0);     // 0
    let b = Math.log1p(1);     // 0 (truncated from 0.693)
    let c = Math.log1p(2);     // 1 (truncated from 1.099)
    let d = Math.log1p(6);     // 1 (truncated from 1.946)
    let e = Math.log1p(9);     // 2 (truncated from 2.303)

    // Result: 0 + 0 + 1 + 1 + 2 = 4
    return a + b + c + d + e;
}
