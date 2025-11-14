// Test string substring method
function main(): number {
    let str = "Hello World";
    let sub = str.substring(0, 5);  // Should be "Hello"

    // Return the length of substring to verify it works
    let len = sub.length;
    return len;  // Should return 5
}
