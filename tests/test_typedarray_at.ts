// Test TypedArray.prototype.at()

function main(): number {
    let arr = new Int32Array(5);
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;
    arr[4] = 50;

    // Test positive index
    let v0 = arr.at(0);
    console.log("at(0):", v0);
    if (v0 !== 10) return 1;

    let v2 = arr.at(2);
    console.log("at(2):", v2);
    if (v2 !== 30) return 2;

    // Test negative index
    let vLast = arr.at(-1);
    console.log("at(-1):", vLast);
    if (vLast !== 50) return 3;

    let vSecondLast = arr.at(-2);
    console.log("at(-2):", vSecondLast);
    if (vSecondLast !== 40) return 4;

    console.log("All TypedArray.at tests passed!");
    return 0;
}
