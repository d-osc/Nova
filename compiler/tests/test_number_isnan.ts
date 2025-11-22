// Test Number.isNaN()
function main(): number {
    let a = Number.isNaN(42);       // false (0)
    let b = Number.isNaN(0);        // false (0)
    let c = Number.isNaN(-5);       // false (0)

    // For integers, nothing is NaN
    // Result: 0 + 0 + 0 = 0
    return a + b + c;
}
