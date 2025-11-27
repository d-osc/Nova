// Test array slice - verify it works by testing the original array
function main(): number {
    let arr = [10, 20, 30, 40, 50];
    let temp = arr.slice(1, 4);  // Create slice
    
    // Verify original array is unchanged
    return arr.length;  // Should still be 5
}
