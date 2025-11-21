// Test for-of loop with break
function main(): number {
    let arr = [10, 20, 30, 40, 50];
    let sum = 0;

    for (let value of arr) {
        if (value >= 30) {
            break;  // Stop at 30
        }
        sum = sum + value;
    }

    return sum;  // Should be 30 (10 + 20)
}
