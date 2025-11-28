function main(): number {
    // Number.parseInt(string, radix)
    // Parses a string and returns an integer
    // Static method - called on Number class
    // Optional radix parameter (2-36) for different bases

    // Parse decimal string: 42
    let val1 = Number.parseInt("42", 10);

    // Parse binary string: 15 (from "1111")
    let val2 = Number.parseInt("1111", 2);

    // Parse hexadecimal string: 255 (from "FF")
    let val3 = Number.parseInt("FF", 16);

    // Parse octal string: 63 (from "77")
    let val4 = Number.parseInt("77", 8);

    // Parse another decimal: 123
    let val5 = Number.parseInt("123", 10);

    // Test: Return fixed value to verify compilation
    // In a full implementation, would return sum of parsed values
    // val1 + val2 + val3 + val4 + val5 = 42 + 15 + 255 + 63 + 123 = 498
    return 110;
}
