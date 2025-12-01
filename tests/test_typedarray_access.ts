// Test TypedArray element access

function main(): number {
    // Create Uint8Array with length
    let arr = new Uint8Array(4);

    // Set elements
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;

    // Read elements
    console.log("arr[0] =", arr[0]);
    console.log("arr[1] =", arr[1]);
    console.log("arr[2] =", arr[2]);
    console.log("arr[3] =", arr[3]);

    // Verify values
    if (arr[0] !== 10) return 1;
    if (arr[1] !== 20) return 2;
    if (arr[2] !== 30) return 3;
    if (arr[3] !== 40) return 4;

    console.log("All tests passed!");
    return 0;
}
