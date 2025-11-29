// Test TypedArray.prototype.forEach()

let sum: number = 0;

function main(): number {
    let arr = new Int32Array(5);
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 4;
    arr[4] = 5;

    // Test forEach - sum all elements
    arr.forEach((x) => {
        sum = sum + x;
        return 0;
    });

    console.log("Sum via forEach:", sum);
    if (sum !== 15) return 1;

    console.log("All TypedArray.forEach tests passed!");
    return 0;
}
