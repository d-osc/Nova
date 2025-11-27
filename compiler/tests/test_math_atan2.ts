function main(): number {
    // Math.atan2(y, x) returns the angle in radians between the positive x-axis
    // and the point (x, y)
    // Result is in radians, between -π and π
    // For integer type system, returns integer approximation
    // atan2(0, 1) = 0 (pointing right)
    // atan2(1, 0) ≈ 1.571 (π/2, pointing up) → 1 as integer
    // atan2(1, 1) ≈ 0.785 (π/4, 45 degrees) → 0 as integer
    // atan2(-1, -1) ≈ -2.356 (-3π/4) → -2 as integer
    // Test basic functionality
    let a = Math.atan2(0, 1);       // 0
    let b = Math.atan2(1, 0);       // 1 (truncated from 1.571)
    let c = Math.atan2(1, 1);       // 0 (truncated from 0.785)
    let d = Math.atan2(-1, -1);     // -2 (truncated from -2.356)
    // Result: 0 + 1 + 0 + (-2) = -1
    return a + b + c + d;
}
