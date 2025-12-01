// Test DataView methods

function main(): number {
    // Create ArrayBuffer
    let buffer = new ArrayBuffer(16);
    let view = new DataView(buffer);

    // Test setInt8/getInt8
    view.setInt8(0, 42);
    view.setInt8(1, -10);
    let v1 = view.getInt8(0);
    let v2 = view.getInt8(1);
    console.log("getInt8(0):", v1);
    console.log("getInt8(1):", v2);
    if (v1 !== 42) return 1;
    if (v2 !== -10) return 2;

    // Test setUint8/getUint8
    view.setUint8(2, 255);
    let v3 = view.getUint8(2);
    console.log("getUint8(2):", v3);
    if (v3 !== 255) return 3;

    // Test setInt32/getInt32
    view.setInt32(4, 12345678);
    let v4 = view.getInt32(4);
    console.log("getInt32(4):", v4);
    if (v4 !== 12345678) return 4;

    // Test setInt16/getInt16
    view.setInt16(8, -1000);
    let v5 = view.getInt16(8);
    console.log("getInt16(8):", v5);
    if (v5 !== -1000) return 5;

    console.log("All DataView tests passed!");
    return 0;
}
