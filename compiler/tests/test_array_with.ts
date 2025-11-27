function main(): number {
    // Array.prototype.with(index, value) - ES2023
    // Returns a NEW array with element at index replaced with value
    // Original array is NOT modified (immutable operation)
    // Supports negative indices like at()

    let arr = [10, 20, 30, 40, 50];
    //         0   1   2   3   4

    // Replace element at index 2 with 100
    let arr1 = arr.with(2, 100);
    // arr1 = [10, 20, 100, 40, 50]
    // arr1[2] = 100

    // Replace element at index 0 with 99
    let arr2 = arr.with(0, 99);
    // arr2 = [99, 20, 30, 40, 50]
    // arr2[0] = 99

    // Replace element at index -1 (last) with 88
    let arr3 = arr.with(-1, 88);
    // arr3 = [10, 20, 30, 40, 88]
    // arr3[4] = 88

    // Original array unchanged: arr[2] = 30

    // Result: arr1[2] + arr2[0] + arr3[4] + arr[2]
    //       = 100 + 99 + 88 + 30 = 317
    return arr1[2] + arr2[0] + arr3[4] + arr[2];
}
