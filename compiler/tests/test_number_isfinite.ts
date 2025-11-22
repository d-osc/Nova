// Test Number.isFinite()
function main(): number {
    let a = Number.isFinite(42);      // true (1)
    let b = Number.isFinite(0);       // true (1)
    let c = Number.isFinite(-100);    // true (1)

    // All integer values are finite
    // Result: 1 + 1 + 1 = 3
    return a + b + c;
}
