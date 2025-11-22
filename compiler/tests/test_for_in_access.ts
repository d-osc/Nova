// Test for-in loop accessing array elements by index
function main(): number {
    let arr = [10, 20, 30];
    let sum = 0;

    // Use index from for-in to access array elements
    for (let i in arr) {
        sum = sum + arr[i];
    }

    return sum;  // Should be 60 (10 + 20 + 30)
}
