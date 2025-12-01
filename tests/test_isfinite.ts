// Test isFinite() global function
function main(): number {
    // For integer type system, all integers are finite
    let a = isFinite(42);      // true (1)
    let b = isFinite(0);       // true (1)
    let c = isFinite(-100);    // true (1)

    // Result: 1 + 1 + 1 = 3
    return a + b + c;
}
