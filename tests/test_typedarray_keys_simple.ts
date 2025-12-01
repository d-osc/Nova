// Simple test for TypedArray.prototype.keys()

function main(): number {
    let arr = new Int32Array(3);
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;

    // Get keys (indices)
    let keys = arr.keys();

    // Just check length
    console.log("Keys length:", keys.length);

    if (keys.length !== 3) return 1;

    console.log("Test passed!");
    return 0;
}
