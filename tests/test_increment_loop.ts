// Test increment/decrement in loops
function main(): number {
    let sum = 0;

    // Test postfix increment in for loop
    for (let i = 0; i < 5; i++) {
        sum = sum + i;  // 0 + 1 + 2 + 3 + 4 = 10
    }

    // Test prefix decrement
    let count = 5;
    while (count > 0) {
        sum = sum + count;  // 10 + 5 + 4 + 3 + 2 + 1 = 25
        --count;
    }

    // sum = 10 + 15 = 25
    return sum;
}
