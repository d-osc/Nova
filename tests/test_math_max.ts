function main(): number {
    // Math.max(a, b) - returns the larger of two values (ES1)

    // Test with positive numbers
    let result1 = Math.max(5, 3);
    console.log(result1);  // Should print 5

    // Test with negative numbers
    let result2 = Math.max(-5, -3);
    console.log(result2);  // Should print -3

    // Test with mixed signs
    let result3 = Math.max(-10, 10);
    console.log(result3);  // Should print 10

    // Test with equal values
    let result4 = Math.max(7, 7);
    console.log(result4);  // Should print 7

    // Test with zero
    let result5 = Math.max(0, -5);
    console.log(result5);  // Should print 0

    // Return success code
    return 191;
}
