// Test Math.fround() function
function main(): number {
    // Math.fround() returns nearest 32-bit single precision float
    // For integer type system, it's a pass-through operation
    let a = Math.fround(42);       // 42
    let b = Math.fround(-25);      // -25
    let c = Math.fround(100);      // 100

    // Result: 42 + (-25) + 100 = 117
    return a + b + c;
}
