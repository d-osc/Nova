// Test array.indexOf() method
function main(): number {
    let arr = [10, 20, 30, 40, 50];

    // Test indexOf() - returns index or -1 if not found
    let idx1 = arr.indexOf(30);   // Should be 2
    let idx2 = arr.indexOf(10);   // Should be 0
    let idx3 = arr.indexOf(50);   // Should be 4
    let idx4 = arr.indexOf(100);  // Should be -1 (not found)

    // Return sum: 2 + 0 + 4 + (-1) = 5
    return idx1 + idx2 + idx3 + idx4;
}
