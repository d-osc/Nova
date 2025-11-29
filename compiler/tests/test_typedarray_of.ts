// Test TypedArray.of() static method

function main(): number {
    // Test Int32Array.of()
    let int32 = Int32Array.of(10, 20, 30, 40);
    console.log("Int32Array.of() length:", int32.length);
    console.log("int32[0]:", int32[0]);
    console.log("int32[1]:", int32[1]);
    console.log("int32[2]:", int32[2]);
    console.log("int32[3]:", int32[3]);

    if (int32.length !== 4) return 1;
    if (int32[0] !== 10) return 2;
    if (int32[1] !== 20) return 3;
    if (int32[2] !== 30) return 4;
    if (int32[3] !== 40) return 5;

    // Test Uint8Array.of()
    let uint8 = Uint8Array.of(100, 200, 255);
    console.log("Uint8Array.of() length:", uint8.length);
    if (uint8.length !== 3) return 6;
    if (uint8[0] !== 100) return 7;
    if (uint8[2] !== 255) return 8;

    // Test Float64Array.of()
    let float64 = Float64Array.of(1, 2, 3, 4, 5);
    console.log("Float64Array.of() length:", float64.length);
    if (float64.length !== 5) return 9;
    if (float64[0] !== 1) return 10;

    console.log("All TypedArray.of() tests passed!");
    return 0;
}
