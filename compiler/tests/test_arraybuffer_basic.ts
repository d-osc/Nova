// Test basic ArrayBuffer functionality

function main(): number {
    // Test 1: Create ArrayBuffer
    let buffer = new ArrayBuffer(16);
    console.log("Created ArrayBuffer with 16 bytes");

    // Test 2: Create Uint8Array view
    let uint8 = new Uint8Array(buffer);
    uint8[0] = 42;
    uint8[1] = 100;
    uint8[15] = 255;

    console.log("Uint8Array[0] =", uint8[0]);
    console.log("Uint8Array[1] =", uint8[1]);
    console.log("Uint8Array[15] =", uint8[15]);

    if (uint8[0] !== 42) return 1;
    if (uint8[1] !== 100) return 2;
    if (uint8[15] !== 255) return 3;

    // Test 3: Create Int32Array view
    let int32 = new Int32Array(buffer);
    int32[0] = 12345;
    console.log("Int32Array[0] =", int32[0]);

    if (int32[0] !== 12345) return 4;

    // Test 4: Create Float64Array view
    let float64 = new Float64Array(buffer);
    float64[0] = 3.14159;
    console.log("Float64Array[0] =", float64[0]);

    console.log("All ArrayBuffer tests passed!");
    return 0;
}
