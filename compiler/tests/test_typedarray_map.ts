// Test TypedArray.prototype.map()

function main(): number {
    let arr = new Int32Array(4);
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 4;

    // Test map - double each element
    let doubled = arr.map((x) => x * 2);

    console.log("Original [0]:", arr[0]);
    console.log("Doubled [0]:", doubled[0]);
    console.log("Doubled [1]:", doubled[1]);
    console.log("Doubled [2]:", doubled[2]);
    console.log("Doubled [3]:", doubled[3]);

    if (doubled[0] !== 2) return 1;
    if (doubled[1] !== 4) return 2;
    if (doubled[2] !== 6) return 3;
    if (doubled[3] !== 8) return 4;

    // Verify original unchanged
    if (arr[0] !== 1) return 5;

    console.log("All TypedArray.map tests passed!");
    return 0;
}
