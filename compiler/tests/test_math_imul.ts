// Test Math.imul() function
function main(): number {
    // Math.imul() performs 32-bit integer multiplication
    let a = Math.imul(2, 3);      // 6
    let b = Math.imul(5, 4);      // 20
    let c = Math.imul(10, 10);    // 100

    // Result: 6 + 20 + 100 = 126
    return a + b + c;
}
