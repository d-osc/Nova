function main(): number {
    // Array.prototype.flat()
    // Flattens nested arrays one level deep (ES2019)
    // Returns a new array with sub-array elements concatenated
    // Example: [1, [2, 3], 4] -> [1, 2, 3, 4]

    let arr = [10, 20, 30];
    // Simulating nested array concept by using flat on regular array
    // For regular array, flat() returns a copy
    // arr.flat() = [10, 20, 30]

    let flattened = arr.flat();

    // Test: flattened.length = 3
    // Test: flattened[1] = 20
    return flattened[1];
}
