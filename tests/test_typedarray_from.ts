// Test TypedArray.from() static method

function main(): number {
    // Create a regular array
    let arr = [1, 2, 3, 4, 5];

    // Test Int32Array.from()
    let int32 = Int32Array.from(arr);
    console.log("Int32Array.from() length:", int32.length);
    console.log("int32[0]:", int32[0]);
    console.log("int32[2]:", int32[2]);
    console.log("int32[4]:", int32[4]);

    if (int32.length !== 5) return 1;
    if (int32[0] !== 1) return 2;
    if (int32[2] !== 3) return 3;
    if (int32[4] !== 5) return 4;

    // Test Uint8Array.from()
    let uint8 = Uint8Array.from(arr);
    console.log("Uint8Array.from() length:", uint8.length);
    if (uint8.length !== 5) return 5;
    if (uint8[0] !== 1) return 6;

    console.log("All TypedArray.from() tests passed!");
    return 0;
}
