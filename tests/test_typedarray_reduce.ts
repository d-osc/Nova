// Test TypedArray.prototype.reduce()

function main(): number {
    let arr = new Int32Array(5);
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 4;
    arr[4] = 5;

    // Test reduce - sum all elements
    let sum = arr.reduce((acc, x) => acc + x, 0);

    console.log("Sum:", sum);

    if (sum !== 15) return 1;

    // Test reduce with different initial value
    let sum100 = arr.reduce((acc, x) => acc + x, 100);
    console.log("Sum with initial 100:", sum100);
    if (sum100 !== 115) return 2;

    console.log("All TypedArray.reduce tests passed!");
    return 0;
}
