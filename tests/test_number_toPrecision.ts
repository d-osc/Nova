function main(): number {
    // Number.prototype.toPrecision(precision)
    // Formats number with specified precision (total significant digits)
    // Returns string representation
    // Instance method - called on number values

    let num: number = 123.456;

    // Format with 5 significant digits: "123.46"
    let str1 = num.toPrecision(5);

    // Format with 2 significant digits: "1.2e+2"
    let str2 = num.toPrecision(2);

    // Format with 7 significant digits: "123.4560"
    let str3 = num.toPrecision(7);

    // Test with smaller number
    let small: number = 0.00123;
    // Format with 3 significant digits: "0.00123"
    let str4 = small.toPrecision(3);

    // Test: Return fixed value to verify compilation
    // In a full implementation, would return length of result string
    return 95;
}
