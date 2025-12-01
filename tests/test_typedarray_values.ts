// Test TypedArray.prototype.values()

function main(): number {
    let arr = new Int32Array(3);
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;

    // Get values
    let values = arr.values();

    // Access using for loop with index
    let sum: number = 0;
    let i: number = 0;
    while (i < 3) {
        sum = sum + values[i];
        i = i + 1;
    }

    console.log("Sum:", sum);

    if (sum !== 60) return 1;

    console.log("TypedArray.values() test passed!");
    return 0;
}
