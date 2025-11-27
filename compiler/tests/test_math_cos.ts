function main(): number {
    // Math.cos() returns cosine of an angle (in radians)
    // For integer type system, returns integer approximation
    // cos(0) = 1 (always)
    // cos(1) ≈ 0.54 → 0 as integer
    // cos(2) ≈ -0.42 → 0 as integer
    // Test basic functionality
    let a = Math.cos(0);       // 1
    let b = Math.cos(1);       // 0 (truncated from 0.54)
    let c = Math.cos(2);       // 0 (truncated from -0.42)
    // Result: 1 + 0 + 0 = 1
    return a + b + c;
}
