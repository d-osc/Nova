// Test for-of loop with continue
function main(): number {
    let arr = [10, 20, 30, 40, 50];
    let sum = 0;

    for (let value of arr) {
        if (value == 30) {
            continue;  // Skip 30
        }
        sum = sum + value;
    }

    return sum;  // Should be 120 (10 + 20 + 40 + 50)
}
