// Test Math.cbrt() function
function main(): number {
    // Math.cbrt() returns cube root
    // For integer type system, returns integer approximation
    let a = Math.cbrt(8);       // 2
    let b = Math.cbrt(27);      // 3
    let c = Math.cbrt(64);      // 4
    let d = Math.cbrt(125);     // 5

    // Result: 2 + 3 + 4 + 5 = 14
    return a + b + c + d;
}
