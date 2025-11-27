function main(): number {
    // String.prototype.at() - get character at index (supports negative indices)
    // Similar to charAt() but supports negative indexing from end
    // For negative indices: at(-1) is last char, at(-2) is second-to-last, etc.
    // Returns character code for testing purposes

    let str = "hello";

    // Test positive indices
    let a = str.at(0);  // 'h' = 104
    let b = str.at(1);  // 'e' = 101

    // Test negative indices
    let c = str.at(-1); // 'o' = 111 (last character)
    let d = str.at(-2); // 'l' = 108 (second-to-last)

    // Result: 104 + 101 + 111 + 108 = 424
    return a + b + c + d;
}
