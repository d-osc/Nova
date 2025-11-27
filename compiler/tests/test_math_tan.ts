function main(): number {
    // Math.tan() returns tangent of an angle (in radians)
    // For integer type system, returns integer approximation
    // tan(0) = 0 (always)
    // tan(1) ≈ 1.557 → 1 as integer
    // tan(2) ≈ -2.185 → -2 as integer
    // Test basic functionality
    let a = Math.tan(0);       // 0
    let b = Math.tan(1);       // 1 (truncated from 1.557)
    let c = Math.tan(2);       // -2 (truncated from -2.185)
    // Result: 0 + 1 + (-2) = -1
    return a + b + c;
}
