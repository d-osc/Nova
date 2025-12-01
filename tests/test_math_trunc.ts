// Test Math.trunc() function
function main(): number {
    // Math.trunc() removes the decimal part of a number
    // For integer type system, it's a pass-through operation
    let a = Math.trunc(42);      // 42
    let b = Math.trunc(-25);     // -25
    let c = Math.trunc(100);     // 100
    let d = Math.trunc(0);       // 0

    // Result: 42 + (-25) + 100 + 0 = 117
    return a + b + c + d;
}
