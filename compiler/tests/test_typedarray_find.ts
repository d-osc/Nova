// Test TypedArray.prototype.find() and findIndex()

function main(): number {
    let arr = new Int32Array(5);
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;
    arr[4] = 50;

    // Test find - find first element > 25
    let found = arr.find((x) => x > 25);
    console.log("find(x > 25):", found);
    if (found !== 30) return 1;

    // Test findIndex - find index of first element > 25
    let idx = arr.findIndex((x) => x > 25);
    console.log("findIndex(x > 25):", idx);
    if (idx !== 2) return 2;

    // Test find not found
    let notFound = arr.find((x) => x > 100);
    console.log("find(x > 100):", notFound);
    if (notFound !== 0) return 3;  // undefined -> 0

    // Test findIndex not found
    let notFoundIdx = arr.findIndex((x) => x > 100);
    console.log("findIndex(x > 100):", notFoundIdx);
    if (notFoundIdx !== -1) return 4;

    console.log("All TypedArray.find/findIndex tests passed!");
    return 0;
}
