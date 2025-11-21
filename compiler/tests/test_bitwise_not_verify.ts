// Test bitwise NOT operator with positive result
function main(): number {
    // ~(-1) should give 0
    // ~(-11) should give 10
    let a = ~(-11);     // 10

    // For two's complement verification
    // ~0 = -1, but we can't return negative
    // So let's use: -~x - 1 = x
    // -~5 - 1 = 5
    let b = -~5 - 1;    // 5

    // Result: 10 + 5 = 15
    return a + b;
}
