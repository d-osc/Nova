// Test TypedArray.prototype.toSorted() - creates a sorted copy

function main(): number {
    let arr = new Int32Array(4);
    arr[0] = 40;
    arr[1] = 10;
    arr[2] = 30;
    arr[3] = 20;

    let sorted = arr.toSorted();

    // Check original is unchanged
    console.log("Original [0]:", arr[0]);
    if (arr[0] !== 40) return 1;

    // Check sorted copy
    console.log("Sorted [0]:", sorted[0]);
    console.log("Sorted [1]:", sorted[1]);
    console.log("Sorted [2]:", sorted[2]);
    console.log("Sorted [3]:", sorted[3]);
    if (sorted[0] !== 10) return 2;
    if (sorted[1] !== 20) return 3;
    if (sorted[2] !== 30) return 4;
    if (sorted[3] !== 40) return 5;

    console.log("All TypedArray.toSorted tests passed!");
    return 0;
}
