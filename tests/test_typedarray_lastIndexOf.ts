// Test TypedArray.prototype.lastIndexOf()

function main(): number {
    let arr = new Int32Array(6);
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 2;  // Duplicate
    arr[4] = 4;
    arr[5] = 2;  // Another duplicate

    // Test basic lastIndexOf
    let idx = arr.lastIndexOf(2);
    console.log("lastIndexOf(2):", idx);
    if (idx !== 5) return 1;

    // Test with fromIndex
    let idx2 = arr.lastIndexOf(2, 4);
    console.log("lastIndexOf(2, 4):", idx2);
    if (idx2 !== 3) return 2;

    // Test not found
    let idx3 = arr.lastIndexOf(99);
    console.log("lastIndexOf(99):", idx3);
    if (idx3 !== -1) return 3;

    console.log("All TypedArray.lastIndexOf tests passed!");
    return 0;
}
