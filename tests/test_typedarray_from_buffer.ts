// Test TypedArray created from ArrayBuffer

function main(): number {
    // Create ArrayBuffer
    let buffer = new ArrayBuffer(16);
    console.log("Created ArrayBuffer with 16 bytes");

    // Create Uint8Array from buffer
    let uint8 = new Uint8Array(buffer);

    // Set values via Uint8Array
    uint8[0] = 42;
    uint8[1] = 100;

    console.log("Set uint8[0] = 42, uint8[1] = 100");

    // Verify values
    if (uint8[0] !== 42) {
        console.log("ERROR: uint8[0] mismatch");
        return 1;
    }
    if (uint8[1] !== 100) {
        console.log("ERROR: uint8[1] mismatch");
        return 2;
    }

    // Create Int32Array from same buffer
    let int32 = new Int32Array(buffer);

    // Values set via uint8 should be visible via int32
    // uint8[0]=42, uint8[1]=100 -> int32[0] = 42 + (100<<8) = 42 + 25600 = 25642
    let expected = 42 + 100 * 256;
    console.log("Int32 value from shared buffer:", int32[0]);
    console.log("Expected:", expected);

    if (int32[0] !== expected) {
        console.log("ERROR: int32[0] mismatch, shared buffer not working");
        return 3;
    }

    console.log("All TypedArray from ArrayBuffer tests passed!");
    return 0;
}
