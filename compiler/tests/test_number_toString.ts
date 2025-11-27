function main(): number {
    // Number.prototype.toString(radix)
    // Converts number to string representation
    // Optional radix parameter (2-36) for different bases
    // Instance method - called on number values

    let num: number = 255;

    // Default base 10: "255"
    let str1 = num.toString(10);

    // Binary (base 2): "11111111"
    let str2 = num.toString(2);

    // Hexadecimal (base 16): "ff"
    let str3 = num.toString(16);

    // Octal (base 8): "377"
    let str4 = num.toString(8);

    // Test with different number
    let num2: number = 42;
    // Base 10: "42"
    let str5 = num2.toString(10);

    // Test: Return fixed value to verify compilation
    // In a full implementation, would return length of result string
    return 100;
}
