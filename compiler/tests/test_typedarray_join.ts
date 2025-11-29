// Test TypedArray.prototype.join()

function main(): number {
    let arr = new Int32Array(4);
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 4;

    // Test default separator (comma)
    let result = arr.join();
    console.log("join():", result);

    // Test custom separator
    let result2 = arr.join("-");
    console.log("join('-'):", result2);

    console.log("All TypedArray.join tests passed!");
    return 0;
}
