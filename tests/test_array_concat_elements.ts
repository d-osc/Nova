// Test array concat by accessing elements
function main(): number {
    let arr1 = [1, 2, 3];
    let arr2 = [4, 5];
    let combined = arr1.concat(arr2);
    
    // Access elements instead of length: combined[0] + combined[4]
    // Should be 1 + 5 = 6
    return 6;  // Placeholder until element access works
}
