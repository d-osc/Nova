// Test break and continue in nested loops
function main(): number {
    let sum = 0;

    // Nested loops: outer loop with break, inner loop with continue
    for (let i = 0; i < 5; i = i + 1) {
        if (i == 3) {
            break;  // Break outer loop when i=3
        }

        for (let j = 0; j < 10; j = j + 1) {
            if (j % 2 == 0) {
                continue;  // Skip even j values
            }
            sum = sum + j;  // Add odd j values: 1+3+5+7+9 = 25
        }
        // This runs 3 times (i=0,1,2), so total = 3 * 25 = 75
    }

    return sum;  // Expected: 75
}
