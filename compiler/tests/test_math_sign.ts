// Test Math.sign() function
function main(): number {
    // Math.sign() returns the sign of a number
    // Returns 1 for positive, -1 for negative, 0 for zero
    let a = Math.sign(42);      // 1
    let b = Math.sign(-25);     // -1
    let c = Math.sign(0);       // 0
    let d = Math.sign(100);     // 1
    let e = Math.sign(-50);     // -1

    // Result: 1 + (-1) + 0 + 1 + (-1) = 0
    return a + b + c + d + e;
}
