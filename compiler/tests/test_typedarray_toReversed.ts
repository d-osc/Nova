// Test TypedArray.prototype.toReversed() - creates a copy

function main(): number {
    let arr = new Int32Array(4);
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 4;

    let reversed = arr.toReversed();

    // Check original is unchanged
    console.log("Original [0]:", arr[0]);
    if (arr[0] !== 1) return 1;

    // Check reversed
    console.log("Reversed [0]:", reversed[0]);
    console.log("Reversed [3]:", reversed[3]);
    if (reversed[0] !== 4) return 2;
    if (reversed[3] !== 1) return 3;

    console.log("All TypedArray.toReversed tests passed!");
    return 0;
}
