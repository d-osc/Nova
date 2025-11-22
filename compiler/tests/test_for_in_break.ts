// Test for-in loop with break
function main(): number {
    let arr = [10, 20, 30, 40, 50];
    let sum = 0;

    for (let i in arr) {
        if (i >= 3) {
            break;  // Stop at index 3
        }
        sum = sum + arr[i];
    }

    return sum;  // Should be 60 (10 + 20 + 30)
}
