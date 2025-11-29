// Test TypedArray.prototype.filter()

function main(): number {
    let arr = new Int32Array(5);
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 4;
    arr[4] = 5;

    // Test filter - keep even numbers
    let evens = arr.filter((x) => x % 2 === 0);

    console.log("Evens length:", evens.length);
    console.log("Evens [0]:", evens[0]);
    console.log("Evens [1]:", evens[1]);

    if (evens.length !== 2) return 1;
    if (evens[0] !== 2) return 2;
    if (evens[1] !== 4) return 3;

    console.log("All TypedArray.filter tests passed!");
    return 0;
}
