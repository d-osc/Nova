// Test TypedArray methods

function main(): number {
    // Test fill
    let arr = new Uint8Array(5);
    arr.fill(42);
    console.log("After fill(42):");
    if (arr[0] !== 42) return 1;
    if (arr[4] !== 42) return 2;

    // Test indexOf
    arr[2] = 100;
    let idx = arr.indexOf(100);
    console.log("indexOf(100):", idx);
    if (idx !== 2) return 3;

    // Test includes
    let has100 = arr.includes(100);
    let has200 = arr.includes(200);
    console.log("includes(100):", has100);
    console.log("includes(200):", has200);
    if (has100 !== 1) return 4;
    if (has200 !== 0) return 5;

    // Test reverse
    let rev = new Uint8Array(4);
    rev[0] = 1;
    rev[1] = 2;
    rev[2] = 3;
    rev[3] = 4;
    rev.reverse();
    console.log("After reverse:");
    if (rev[0] !== 4) return 6;
    if (rev[3] !== 1) return 7;

    console.log("All TypedArray method tests passed!");
    return 0;
}
