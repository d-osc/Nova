// Test isNaN() global function
function main(): number {
    // For integer type system, all integers are not NaN
    let a = isNaN(42);      // false (0)
    let b = isNaN(0);       // false (0)
    let c = isNaN(-100);    // false (0)

    // Result: 0 + 0 + 0 = 0
    return a + b + c;
}
