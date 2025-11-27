function main(): number {
    // String.fromCodePoint(codePoint)
    // Creates string from Unicode code point (ES2015)
    // Static method - called on String class, not string instances

    // Create string from code point 66 (character 'B')
    let str = String.fromCodePoint(66);

    // str should be "B"
    // Test: get the character code back
    let code = str.charCodeAt(0);

    // Test: should return 66
    return code;
}
