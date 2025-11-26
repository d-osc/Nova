// Test String.includes()
function main(): number {
    let str = "Hello World";
    let has1 = str.includes("World");   // Should be true (1)
    let has2 = str.includes("xyz");     // Should be false (0)

    return has1 + has2;  // Should be 1
}
