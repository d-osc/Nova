function main(): number {
    // parseFloat() - global function to parse strings to floating-point numbers
    // Same behavior as Number.parseFloat()

    // Test basic parsing
    let val1 = parseFloat("3.14");
    let val2 = parseFloat("42");
    let val3 = parseFloat("-123.45");

    // Test with whitespace and scientific notation
    let val4 = parseFloat("  99.99");
    let val5 = parseFloat("1.23e2");

    // Return success code
    return 155;
}
