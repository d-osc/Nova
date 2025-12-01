// Test array find method
function main(): number {
    let arr = [1, 2, 3, 4, 5];
    let found = arr.find((x) => x > 3);
    
    return found;  // Should be 4
}
