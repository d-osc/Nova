function main(): number {
    // Array.prototype.toSpliced(start, deleteCount)
    // Returns new array with elements removed (immutable version of splice)
    // Does not modify original array (ES2023)

    let original = [10, 20, 30, 40, 50];
    //              0   1   2   3   4

    // Remove 2 elements starting from index 1
    let result = original.toSpliced(1, 2);
    // result = [10, 40, 50]
    // original = [10, 20, 30, 40, 50] (unchanged)

    // Test: result[1] should be 40
    return result[1];
}
