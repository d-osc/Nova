// Test optional chaining operator (?.)
function main(): number {
    let obj = { x: 42 };

    // Optional chaining should safely access properties
    let result1 = obj?.x;  // Should be 42

    // This should work even if the object is undefined (but we don't have null/undefined yet)
    let result2 = obj?.x ?? 0;  // Should be 42

    return result1;  // Should be 42
}
