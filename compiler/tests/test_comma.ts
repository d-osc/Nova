// Test comma operator
function main(): number {
    let a = (1, 2, 3);  // Should be 3 (last value)
    let b = (10, 20);   // Should be 20
    return a + b;  // 3 + 20 = 23
}
