// Basic arithmetic test
function main(): number {
    let x = 5;
    let y = 10;
    let sum = x + y;

    // Test: 5 + 10 should equal 15
    if (sum !== 15) {
        return 1;  // FAIL
    }

    let product = x * y;
    // Test: 5 * 10 should equal 50
    if (product !== 50) {
        return 2;  // FAIL
    }

    // All tests passed
    return 0;
}
