// Test TypedArray element access with verification

function main(): number {
    // Create Uint8Array with length
    let arr = new Uint8Array(4);

    // Set elements
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;

    // Verify values
    let sum = arr[0] + arr[1] + arr[2] + arr[3];
    console.log("Sum should be 100:", sum);

    if (sum !== 100) {
        console.log("ERROR: Sum mismatch");
        return 1;
    }

    // Test Int32Array
    let int32 = new Int32Array(2);
    int32[0] = 1000000;
    int32[1] = -500000;

    let int32sum = int32[0] + int32[1];
    console.log("Int32 sum should be 500000:", int32sum);

    if (int32sum !== 500000) {
        console.log("ERROR: Int32 sum mismatch");
        return 2;
    }

    console.log("All TypedArray tests passed!");
    return 0;
}
