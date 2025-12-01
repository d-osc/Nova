// Test nested loops without complex conditionals
function main(): number {
    let sum = 0;

    // Simple nested loops
    for (let i = 0; i < 3; i = i + 1) {
        for (let j = 0; j < 4; j = j + 1) {
            sum = sum + 1;  // This runs 3 * 4 = 12 times
        }
    }

    return sum;  // Expected: 12
}
