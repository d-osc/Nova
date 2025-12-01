// Test Array.isArray function
function main(): number {
    let arr = [1, 2, 3];
    let num = 42;
    
    let result1 = Array.isArray(arr);   // Should be true (1)
    let result2 = Array.isArray(num);   // Should be false (0)
    
    return result1 + result2;  // Should be 1 + 0 = 1
}
