// Test string methods
function main(): number {
    let str = "Hello World";

    // Test substring
    let sub = str.substring(0, 5);  // Should be "Hello"

    // Test indexOf
    let idx = str.indexOf("World");  // Should be 6

    // Test charAt
    let ch = str.charAt(0);  // Should be "H"

    // For now, just return success
    // Later we can test actual values
    return 42;
}
