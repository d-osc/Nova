function main(): number {
    let str = "Hello   ";
    // trimEnd removes trailing whitespace
    let result = str.trimEnd();
    // Result should be "Hello" with length 5
    return result.length;
}
