// Test TypedArray.prototype.with() - ES2023
// Returns a copy with one element replaced

function main(): number {
    let arr = new Int32Array(4);
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;

    // Test basic with() - positive index
    let result = arr.with(1, 99);

    // Check the result has the replaced value
    if (result[0] !== 10) {
        console.log("FAIL: result[0] should be 10");
        return 1;
    }
    if (result[1] !== 99) {
        console.log("FAIL: result[1] should be 99");
        return 2;
    }
    if (result[2] !== 30) {
        console.log("FAIL: result[2] should be 30");
        return 3;
    }
    if (result[3] !== 40) {
        console.log("FAIL: result[3] should be 40");
        return 4;
    }

    // Check original is unchanged (immutable)
    if (arr[1] !== 20) {
        console.log("FAIL: original arr[1] should still be 20");
        return 5;
    }

    // Test with() - negative index
    let result2 = arr.with(-1, 999);
    if (result2[3] !== 999) {
        console.log("FAIL: result2[-1] (index 3) should be 999");
        return 6;
    }

    console.log("All TypedArray.with() tests passed!");
    return 0;
}
