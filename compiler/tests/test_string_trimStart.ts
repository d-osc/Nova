function main(): number {
    let str = "   Hello";
    // trimStart removes leading whitespace
    let result = str.trimStart();
    // Result should be "Hello" with length 5
    return result.length;
}
