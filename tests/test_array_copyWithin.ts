function main(): number {
    // Array.prototype.copyWithin(target, start, end)
    // Shallow copies part of array to another location in same array (ES2015)
    // Modifies array in place and returns it
    // Does not change array length

    let arr = [10, 20, 30, 40, 50];
    //         0   1   2   3   4

    // Copy elements from index 3 to 5 (40, 50) to index 0
    arr.copyWithin(0, 3, 5);
    // arr = [40, 50, 30, 40, 50]
    //        0   1   2   3   4

    // Test: arr[0] = 40
    return arr[0];
}
