// Test for-of loop with arrays
function main(): number {
    let arr = [10, 20, 30];
    let sum = 0;

    for (let value of arr) {
        sum = sum + value;
    }

    return sum;  // Should be 60
}
