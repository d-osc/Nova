// Test TypedArray properties

function main(): number {
    // Create ArrayBuffer
    let buffer = new ArrayBuffer(32);
    console.log("ArrayBuffer byteLength:", buffer.byteLength);

    if (buffer.byteLength !== 32) {
        console.log("ERROR: ArrayBuffer byteLength mismatch");
        return 1;
    }

    // Create Uint8Array from buffer
    let u8 = new Uint8Array(buffer);
    console.log("Uint8Array length:", u8.length);
    console.log("Uint8Array byteLength:", u8.byteLength);
    console.log("Uint8Array byteOffset:", u8.byteOffset);

    if (u8.length !== 32) return 2;
    if (u8.byteLength !== 32) return 3;
    if (u8.byteOffset !== 0) return 4;

    // Create Int32Array from buffer
    let i32 = new Int32Array(buffer);
    console.log("Int32Array length:", i32.length);
    console.log("Int32Array byteLength:", i32.byteLength);

    if (i32.length !== 8) return 5;  // 32 / 4 = 8
    if (i32.byteLength !== 32) return 6;

    // Create standalone TypedArray
    let arr = new Uint16Array(10);
    console.log("Uint16Array length:", arr.length);
    console.log("Uint16Array byteLength:", arr.byteLength);

    if (arr.length !== 10) return 7;
    if (arr.byteLength !== 20) return 8;  // 10 * 2 = 20

    console.log("All TypedArray property tests passed!");
    return 0;
}
