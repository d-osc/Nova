// Test 'in' operator
function main(): number {
    let obj = { x: 42, y: 10 };

    // 'in' operator should check if property exists
    let hasX = "x" in obj;  // Should be true (1)
    let hasZ = "z" in obj;  // Should be false (0)

    // Convert boolean to number for return
    return hasX ? 1 : 0;  // Should be 1
}
