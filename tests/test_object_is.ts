function main(): number {
    // Object.is(value1, value2) - determines if two values are the same
    // Similar to === but handles special cases like NaN

    // Test same values
    let result1 = Object.is(1, 1);
    console.log(result1);  // Should print 1 (true)

    // Test different values
    let result2 = Object.is(1, 2);
    console.log(result2);  // Should print 0 (false)

    // Test same strings
    let result3 = Object.is(5, 5);
    console.log(result3);  // Should print 1 (true)

    // Test zero equality
    let result4 = Object.is(0, 0);
    console.log(result4);  // Should print 1 (true)

    // Test negative values
    let result5 = Object.is(-1, -1);
    console.log(result5);  // Should print 1 (true)

    // Test different signs
    let result6 = Object.is(1, -1);
    console.log(result6);  // Should print 0 (false)

    // Return success code
    return 186;
}
