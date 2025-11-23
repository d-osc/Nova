// Test array.includes() method
function main(): number {
    let arr = [10, 20, 30, 40];

    // Test includes() - should return 1 for true, 0 for false
    let has20 = arr.includes(20);  // Should be 1 (true)
    let has50 = arr.includes(50);  // Should be 0 (false)
    let has10 = arr.includes(10);  // Should be 1 (true)
    let has40 = arr.includes(40);  // Should be 1 (true)

    // Return sum: 1 + 0 + 1 + 1 = 3
    return has20 + has50 + has10 + has40;
}
