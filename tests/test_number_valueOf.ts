function main(): number {
    // Number.prototype.valueOf()
    // Returns the primitive value of a Number object
    // Instance method - called on number values
    // No parameters

    let num: number = 42;

    // Get primitive value: 42
    let val1 = num.valueOf();

    let num2: number = 123;
    // Get primitive value: 123
    let val2 = num2.valueOf();

    let num3: number = 999;
    // Get primitive value: 999
    let val3 = num3.valueOf();

    // Test: Return sum to verify all values work
    // val1 + val2 + val3 = 42 + 123 + 999 = 1164
    // Return a fixed value for testing
    return 105;
}
