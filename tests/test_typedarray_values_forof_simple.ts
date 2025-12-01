// Simple test for TypedArray.prototype.values() with for-of loop

function main(): number {
    let arr = new Int32Array(3);
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;

    let values = arr.values();

    // Just iterate and return first value
    for (let v of values) {
        console.log("v:", v);
        return v;  // Return first value (should be 10)
    }

    return -1;
}
