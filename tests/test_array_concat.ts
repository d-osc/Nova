// Test array concat method
function main(): number {
    let arr1 = [1, 2, 3];
    let arr2 = [4, 5];
    let combined = arr1.concat(arr2);
    
    return combined.length;  // Should be 5
}
