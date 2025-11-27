function main(): number {
    // Math.acos() returns arccosine (inverse cosine) of a number
    // Input must be between -1 and 1
    // Result is in radians, between 0 and π
    // For integer type system, returns integer approximation
    // acos(1) = 0 (always)
    // acos(0) ≈ 1.571 (π/2) → 1 as integer
    // acos(-1) ≈ 3.142 (π) → 3 as integer
    // Test basic functionality
    let a = Math.acos(1);      // 0
    let b = Math.acos(0);      // 1 (truncated from 1.571)
    let c = Math.acos(-1);     // 3 (truncated from 3.142)
    // Result: 0 + 1 + 3 = 4
    return a + b + c;
}
