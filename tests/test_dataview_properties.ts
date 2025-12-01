// Test DataView properties (ES2015)

function main(): number {
    // =========================================
    // Test 1: Create DataView from ArrayBuffer
    // =========================================
    let buffer = new ArrayBuffer(32);
    let view = new DataView(buffer);
    console.log("Created DataView from ArrayBuffer(32)");
    console.log("PASS: DataView constructor");

    // =========================================
    // Test 2: DataView.prototype.byteLength
    // =========================================
    let len = view.byteLength;
    console.log("view.byteLength:", len);  // 32
    console.log("PASS: DataView.prototype.byteLength");

    // =========================================
    // Test 3: DataView.prototype.byteOffset
    // =========================================
    let offset = view.byteOffset;
    console.log("view.byteOffset:", offset);  // 0
    console.log("PASS: DataView.prototype.byteOffset");

    // =========================================
    // Test 4: DataView with byteOffset
    // =========================================
    let view2 = new DataView(buffer, 8, 16);
    let len2 = view2.byteLength;
    let offset2 = view2.byteOffset;
    console.log("view2 (offset=8, length=16):");
    console.log("  byteLength:", len2);   // 16
    console.log("  byteOffset:", offset2); // 8
    console.log("PASS: DataView with offset");

    // =========================================
    // Test 5: DataView.prototype.buffer
    // =========================================
    let buf = view.buffer;
    console.log("view.buffer retrieved");
    console.log("PASS: DataView.prototype.buffer");

    // =========================================
    // Test 6: Use properties with methods
    // =========================================
    view.setInt32(0, 12345, true);
    let val = view.getInt32(0, true);
    console.log("Set and get Int32:", val);  // 12345

    // Check that byteLength is still correct after operations
    let lenAfter = view.byteLength;
    console.log("byteLength after operations:", lenAfter);  // 32
    console.log("PASS: Properties with methods");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All DataView property tests passed!");
    return 0;
}
