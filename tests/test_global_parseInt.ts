function main(): number {
    // parseInt() - global function to parse strings to integers
    // Accepts string and optional radix (base)
    // Same behavior as Number.parseInt()

    // Test basic parsing
    let val1 = parseInt("42");
    let val2 = parseInt("123");
    let val3 = parseInt("7");

    // Test with radix
    let val4 = parseInt("1010", 2);  // Binary: 10
    let val5 = parseInt("FF", 16);   // Hexadecimal: 255

    // Return success code
    return 150;
}
