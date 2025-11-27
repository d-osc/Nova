function main(): number {
    // Array.of(...elements)
    // Creates a new Array with the given elements (ES2015)
    // Static method: called as Array.of(), not array.of()
    // Example: Array.of(1, 2, 3) returns [1, 2, 3]

    // Create array from individual values
    let arr = Array.of(10, 20, 30);
    // arr = [10, 20, 30]
    // arr.length = 3

    // Test: arr[1] = 20
    return arr[1];
}
