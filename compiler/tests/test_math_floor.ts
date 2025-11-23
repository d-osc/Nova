// Test Math.floor() function
function main(): number {
    // Math.floor() rounds down to nearest integer
    // For integer type system, it's a pass-through operation
    let a = Math.floor(42);      // 42
    let b = Math.floor(-25);     // -25
    let c = Math.floor(100);     // 100
    let d = Math.floor(0);       // 0

    // Result: 42 + (-25) + 100 + 0 = 117
    return a + b + c + d;
}
