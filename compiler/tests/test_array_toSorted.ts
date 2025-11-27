function main(): number {
    // Array.prototype.toSorted() - ES2023
    // Returns a NEW sorted array (ascending numeric order)
    // Original array is NOT modified (immutable operation)

    let arr = [50, 10, 40, 20, 30];
    //         0   1   2   3   4

    // Create sorted copy
    let sorted = arr.toSorted();
    // sorted = [10, 20, 30, 40, 50]
    //           0   1   2   3   4

    // Test: sorted[0] = 10 (smallest element)
    return sorted[0];
}
