function main(): number {
    // String.prototype.codePointAt(index)
    // Returns Unicode code point at given index (ES2015)
    // Unlike charCodeAt, handles full Unicode including surrogate pairs

    let str = "ABC";
    //         012

    // Get code point at index 1 (character 'B')
    let code = str.codePointAt(1);
    // 'B' has code point 66

    // Test: should return 66
    return code;
}
