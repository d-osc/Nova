// Test for-in loop with arrays (iterates over indices)
function main(): number {
    let arr = [10, 20, 30];
    let sum = 0;

    // for-in iterates over array indices (0, 1, 2)
    for (let index in arr) {
        sum = sum + index;
    }

    return sum;  // Should be 3 (0 + 1 + 2)
}
