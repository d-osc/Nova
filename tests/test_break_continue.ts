// Test break and continue statements
function main(): number {
    let sum = 0;

    // Test continue: skip even numbers
    for (let i = 0; i < 10; i = i + 1) {
        if (i % 2 == 0) {
            continue;  // Skip even numbers
        }
        sum = sum + i;  // Only odd numbers: 1+3+5+7+9 = 25
    }

    // Test break: stop at 5
    let count = 0;
    for (let j = 0; j < 100; j = j + 1) {
        if (j == 5) {
            break;  // Stop when j reaches 5
        }
        count = count + 1;  // count will be 5 (0,1,2,3,4)
    }

    // Result: sum(25) + count(5) = 30
    return sum + count;
}
