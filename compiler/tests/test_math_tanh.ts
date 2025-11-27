function main(): number {
    // Math.tanh() returns the hyperbolic tangent of a number
    // tanh(x) = sinh(x) / cosh(x) = (e^x - e^-x) / (e^x + e^-x)
    // Result is always between -1 and 1
    // For integer type system, returns integer approximation
    // tanh(0) = 0 (always)
    // tanh(1) ≈ 0.762 → 0 as integer
    // tanh(2) ≈ 0.964 → 0 as integer
    // Note: All tanh values truncate to 0 for positive arguments < ~2
    // because tanh output is bounded between -1 and 1
    let a = Math.tanh(0);      // 0
    let b = 10 * Math.tanh(1); // 10 * 0 = 0 (tanh(1) truncates to 0)
    let c = 10 * Math.tanh(2); // 10 * 0 = 0 (tanh(2) truncates to 0)
    // Result: 0 + 0 + 0 = 0
    return a + b + c;
}
