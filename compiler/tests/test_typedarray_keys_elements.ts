// Test TypedArray.prototype.keys() element access

function main(): number {
    let arr = new Int32Array(3);
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;

    let keys = arr.keys();

    // Check length first
    if (keys.length !== 3) return 1;

    // Try to access elements
    let k0 = keys[0];
    console.log("k0:", k0);
    if (k0 !== 0) return 2;

    let k1 = keys[1];
    console.log("k1:", k1);
    if (k1 !== 1) return 3;

    console.log("Test passed!");
    return 0;
}
