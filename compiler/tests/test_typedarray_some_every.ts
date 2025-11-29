// Test TypedArray.prototype.some() and every()

function main(): number {
    let arr = new Int32Array(5);
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 4;
    arr[4] = 5;

    // Test some - check if any element > 3
    let hasLarge = arr.some((x) => x > 3);
    console.log("some(x > 3):", hasLarge);
    if (hasLarge !== 1) return 1;

    // Test some - check if any element > 10 (should be false)
    let hasVeryLarge = arr.some((x) => x > 10);
    console.log("some(x > 10):", hasVeryLarge);
    if (hasVeryLarge !== 0) return 2;

    // Test every - check if all elements > 0
    let allPositive = arr.every((x) => x > 0);
    console.log("every(x > 0):", allPositive);
    if (allPositive !== 1) return 3;

    // Test every - check if all elements > 2 (should be false)
    let allLarge = arr.every((x) => x > 2);
    console.log("every(x > 2):", allLarge);
    if (allLarge !== 0) return 4;

    console.log("All TypedArray.some/every tests passed!");
    return 0;
}
