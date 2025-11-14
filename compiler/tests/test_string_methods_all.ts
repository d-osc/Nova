// Comprehensive string methods test
function main(): number {
    let str = "Hello World";

    // Test substring: "Hello World".substring(0, 5) = "Hello" (length 5)
    let sub = str.substring(0, 5);
    let len1 = sub.length;  // Should be 5

    // Test indexOf: "Hello World".indexOf("World") = 6
    let idx = str.indexOf("World");  // Should be 6

    // Test charAt: "Hello World".charAt(6) = "W" (length 1)
    let ch = str.charAt(6);
    let len2 = ch.length;  // Should be 1

    // Return sum: 5 + 6 + 1 = 12
    return len1 + idx + len2;
}
