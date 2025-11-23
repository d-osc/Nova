// Test Math.sign() function
function main(): number {
    // Math.sign() returns -1, 0, or 1 based on the sign of the number
    let a = Math.sign(42);      // 1 (positive)
    let b = Math.sign(-25);     // -1 (negative)
    let c = Math.sign(0);       // 0 (zero)
    let d = Math.sign(100);     // 1 (positive)
    let e = Math.sign(-50);     // -1 (negative)

    // Result: 1 + (-1) + 0 + 1 + (-1) = 0
    return a + b + c + d + e;
}
