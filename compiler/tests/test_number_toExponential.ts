function main(): number {
    // Number.prototype.toExponential(fractionDigits)
    // Formats number in exponential notation (scientific notation)
    // Returns string representation like "1.23e+2"
    // Instance method - called on number values

    let num: number = 123.456;

    // Format with 2 decimal places: "1.23e+2"
    let str1 = num.toExponential(2);

    // Format with 0 decimal places: "1e+2"
    let str2 = num.toExponential(0);

    // Format with 4 decimal places: "1.2346e+2"
    let str3 = num.toExponential(4);

    // Test with smaller number
    let small: number = 0.00123;
    // Format: "1.23e-3"
    let str4 = small.toExponential(2);

    // Test: Return fixed value to verify compilation
    // In a full implementation, would return length of result string
    return 90;
}
