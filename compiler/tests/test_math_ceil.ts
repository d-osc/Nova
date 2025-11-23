// Test Math.ceil() function
function main(): number {
    // Math.ceil() rounds up to nearest integer
    // For integer type system, it's a pass-through operation
    let a = Math.ceil(42);      // 42
    let b = Math.ceil(-25);     // -25
    let c = Math.ceil(100);     // 100
    let d = Math.ceil(0);       // 0

    // Result: 42 + (-25) + 100 + 0 = 117
    return a + b + c + d;
}
