function main(): number {
    // Array.prototype.splice(start, deleteCount)
    // Removes deleteCount elements starting at start index
    // Modifies the array in place and returns the array

    let arr = [10, 20, 30, 40, 50];
    //         0   1   2   3   4
    // length = 5

    // Remove 2 elements starting at index 1
    // Removes: 20, 30
    arr.splice(1, 2);
    // arr = [10, 40, 50]
    //        0   1   2
    // length = 3

    // Test: arr.length = 3
    return arr.length;
}
