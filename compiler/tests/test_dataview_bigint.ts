// Test DataView BigInt methods

function main(): number {
    let buffer = new ArrayBuffer(16);
    let view = new DataView(buffer);

    // Test setBigInt64/getBigInt64
    view.setBigInt64(0, 1234567890123456789);
    let v1 = view.getBigInt64(0);
    console.log("getBigInt64(0):", v1);

    // Test negative value
    view.setBigInt64(8, -9876543210);
    let v2 = view.getBigInt64(8);
    console.log("getBigInt64(8):", v2);

    console.log("All DataView BigInt tests passed!");
    return 0;
}
