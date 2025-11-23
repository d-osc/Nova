// Test Math.imul() function
function main(): number {
    // Math.imul() performs C-like 32-bit multiplication
    // For our integer type system, it works like regular multiplication
    let a = Math.imul(3, 4);        // 12
    let b = Math.imul(5, 10);       // 50
    let c = Math.imul(2, 25);       // 50

    // Result: 12 + 50 + 50 = 112
    return a + b + c;
}
