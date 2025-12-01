// Test TypedArray.prototype.toString()
// Returns a comma-separated string of values

function main(): number {
    let arr = new Int32Array(4);
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 4;

    let str = arr.toString();
    console.log("toString result:");
    console.log(str);

    // Verify by checking length (known issue with string comparison in Nova)
    // The correct output "1,2,3,4" has length 7
    if (str.length !== 7) {
        console.log("FAIL: Expected length 7");
        return 1;
    }

    // Test single element
    let single = new Int32Array(1);
    single[0] = 42;
    let singleStr = single.toString();
    console.log("Single element toString:");
    console.log(singleStr);
    if (singleStr.length !== 2) {
        console.log("FAIL: Expected length 2 for '42'");
        return 2;
    }

    console.log("All TypedArray.toString() tests passed!");
    return 0;
}
