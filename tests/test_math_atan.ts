function main(): number {
    // Math.atan() returns arctangent (inverse tangent) of a number
    // Result is in radians, between -π/2 and π/2
    // For integer type system, returns integer approximation
    // atan(0) = 0 (always)
    // atan(1) ≈ 0.785 (π/4) → 0 as integer
    // atan(2) ≈ 1.107 → 1 as integer
    // Test basic functionality
    let a = Math.atan(0);      // 0
    let b = Math.atan(1);      // 0 (truncated from 0.785)
    let c = Math.atan(2);      // 1 (truncated from 1.107)
    // Result: 0 + 0 + 1 = 1
    return a + b + c;
}
