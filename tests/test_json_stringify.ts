function main(): number {
    // JSON.stringify(value) - converts a value to a JSON string (ES5)

    // Test with number
    let num = 42;
    let result1 = JSON.stringify(num);
    console.log(result1);  // Should print: 42

    // Test with negative number
    let neg = -17;
    let result2 = JSON.stringify(neg);
    console.log(result2);  // Should print: -17

    // Test with zero
    let zero = 0;
    let result3 = JSON.stringify(zero);
    console.log(result3);  // Should print: 0

    // Return success code
    return 192;
}
