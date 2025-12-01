function main(): number {
    // Math.sin() returns sine of an angle (in radians)
    // For integer type system, returns integer approximation
    // sin(0) = 0 (always)
    // sin(1) ≈ 0.84 → 0 as integer
    // sin(2) ≈ 0.91 → 0 as integer
    // sin(3) ≈ 0.14 → 0 as integer
    // Test basic functionality
    let a = Math.sin(0);       // 0
    let b = Math.sin(1);       // 0 (truncated from 0.84)
    let c = Math.sin(2);       // 0 (truncated from 0.91)
    // Result: 0 + 0 + 0 = 0
    return a + b + c;
}
