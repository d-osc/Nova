// Test typeof operator
function main(): number {
    let num = 42;
    let str = "hello";
    let arr = [1, 2, 3];

    // Test typeof - should return strings
    let t1 = typeof num;      // Should be "number"
    let t2 = typeof str;      // Should be "string"
    let t3 = typeof arr;      // Should be "object"

    // Return length of first typeof result to verify it works
    // "number" has 6 characters
    return t1.length;  // Should return 6
}
