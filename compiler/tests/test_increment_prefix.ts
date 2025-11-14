// Test prefix increment and decrement
function main(): number {
    let x = 5;

    // Prefix increment: ++x returns new value (6)
    let a = ++x;  // x becomes 6, a = 6

    // Prefix decrement: --x returns new value (5)
    let b = --x;  // x becomes 5, b = 5

    // Test that x is now 5
    // a = 6, b = 5, x = 5
    // Return: 6 + 5 + 5 = 16
    return a + b + x;
}
