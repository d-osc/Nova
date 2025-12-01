// Test typeof with boolean
function main(): number {
    let flag = true;
    let t = typeof flag;  // Should be "boolean"

    // "boolean" has 7 characters
    return t.length;  // Should return 7
}
