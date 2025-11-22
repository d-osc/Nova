// Test Number.isInteger()
function main(): number {
    let a = Number.isInteger(42);     // true (1)
    let b = Number.isInteger(0);      // true (1)
    let c = Number.isInteger(-5);     // true (1)
    let d = Number.isInteger(100);    // true (1)

    // All integers are integers in our system
    // Result: 1 + 1 + 1 + 1 = 4
    return a + b + c + d;
}
