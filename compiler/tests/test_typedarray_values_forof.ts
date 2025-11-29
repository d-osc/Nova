// Test TypedArray.prototype.values() with for-of loop

function main(): number {
    let arr = new Int32Array(3);
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;

    // Get values and iterate with for-of
    let values = arr.values();
    let sum: number = 0;

    for (let v of values) {
        sum = sum + v;
    }

    console.log("Sum:", sum);

    if (sum !== 60) return 1;

    console.log("Test passed!");
    return 0;
}
