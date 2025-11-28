function main(): number {
    // JSON.stringify(boolean) - converts a boolean to a JSON string (ES5)

    // Test with true
    let val1 = true;
    let result1 = JSON.stringify(val1);
    console.log(result1);  // Should print: true

    // Test with false
    let val2 = false;
    let result2 = JSON.stringify(val2);
    console.log(result2);  // Should print: false

    // Return success code
    return 194;
}
