function main(): number {
    // Array.prototype.sort() - in-place sorting
    // Sorts the array in ascending numeric order
    // Original array IS modified (mutable operation)

    let arr = [50, 10, 40, 20, 30];
    //         0   1   2   3   4

    // Sort in place
    arr.sort();
    // arr = [10, 20, 30, 40, 50]
    //        0   1   2   3   4

    // Test: arr[0] = 10 (smallest element after sort)
    return arr[0];
}
