// Test postfix increment and decrement
function main(): number {
    let x = 5;

    // Postfix increment: x++ returns old value (5)
    let a = x++;  // returns 5, x becomes 6

    // Postfix decrement: x-- returns old value (6)
    let b = x--;  // returns 6, x becomes 5

    // Test that x is back to 5
    // a = 5, b = 6, x = 5
    // Return: 5 + 6 + 5 = 16
    return a + b + x;
}
