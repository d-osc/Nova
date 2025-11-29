// Test TypedArray.prototype.keys()

function main(): number {
    let arr = new Int32Array(3);
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;

    // Get keys (indices)
    let keys = arr.keys();

    console.log("Keys length:", keys.length);
    console.log("Key 0:", keys[0]);
    console.log("Key 1:", keys[1]);
    console.log("Key 2:", keys[2]);

    if (keys.length !== 3) return 1;
    if (keys[0] !== 0) return 2;
    if (keys[1] !== 1) return 3;
    if (keys[2] !== 2) return 4;

    console.log("TypedArray.keys() test passed!");
    return 0;
}
