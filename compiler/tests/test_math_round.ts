// Test Math.round() function
function main(): number {
    // Math.round() rounds to nearest integer
    // For integer type system, it's a pass-through operation
    let a = Math.round(42);      // 42
    let b = Math.round(-25);     // -25
    let c = Math.round(100);     // 100
    let d = Math.round(0);       // 0

    // Result: 42 + (-25) + 100 + 0 = 117
    return a + b + c + d;
}
