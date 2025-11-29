// Test TypedArray.prototype.sort()

function main(): number {
    let arr = new Int32Array(5);
    arr[0] = 50;
    arr[1] = 10;
    arr[2] = 40;
    arr[3] = 20;
    arr[4] = 30;

    console.log("Before sort:");
    console.log("  [0]:", arr[0]);
    console.log("  [1]:", arr[1]);
    console.log("  [2]:", arr[2]);

    arr.sort();

    console.log("After sort:");
    console.log("  [0]:", arr[0]);
    console.log("  [1]:", arr[1]);
    console.log("  [2]:", arr[2]);
    console.log("  [3]:", arr[3]);
    console.log("  [4]:", arr[4]);

    if (arr[0] !== 10) return 1;
    if (arr[1] !== 20) return 2;
    if (arr[2] !== 30) return 3;
    if (arr[3] !== 40) return 4;
    if (arr[4] !== 50) return 5;

    console.log("All TypedArray.sort tests passed!");
    return 0;
}
