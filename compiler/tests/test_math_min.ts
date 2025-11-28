function main(): number {
    // Math.min(a, b) - returns the smaller of two values (ES1)

    // Test with positive numbers
    let result1 = Math.min(5, 3);
    console.log(result1);  // Should print 3

    // Test with negative numbers
    let result2 = Math.min(-5, -3);
    console.log(result2);  // Should print -5

    // Test with mixed signs
    let result3 = Math.min(-10, 10);
    console.log(result3);  // Should print -10

    // Test with equal values
    let result4 = Math.min(7, 7);
    console.log(result4);  // Should print 7

    // Test with zero
    let result5 = Math.min(0, 5);
    console.log(result5);  // Should print 0

    // Return success code
    return 190;
}
