// Test array slice method
function main(): number {
    let arr = [1, 2, 3, 4, 5];
    let sliced = arr.slice(1, 3);
    
    return sliced.length;  // Should be 2
}
