// Test Math.hypot() function
function main(): number {
    // Math.hypot() returns sqrt(x^2 + y^2 + ...)
    // For integer type system, returns integer approximation
    let a = Math.hypot(3, 4);       // sqrt(9+16) = sqrt(25) = 5
    let b = Math.hypot(5, 12);      // sqrt(25+144) = sqrt(169) = 13
    let c = Math.hypot(8, 15);      // sqrt(64+225) = sqrt(289) = 17

    // Result: 5 + 13 + 17 = 35
    return a + b + c;
}
