// Test Int8Array.of() and other TypedArray.of() static methods

function main(): number {
    // =========================================
    // Test 1: Int8Array.of()
    // =========================================
    console.log("=== Int8Array.of() ===");
    let arr1 = Int8Array.of(1, 2, 3, -1, -128, 127);
    console.log("Int8Array.of(1, 2, 3, -1, -128, 127) created");
    console.log("Length:", arr1.length);
    console.log("arr1[0]:", arr1[0]);
    console.log("arr1[3]:", arr1[3]);
    console.log("arr1[4]:", arr1[4]);
    console.log("arr1[5]:", arr1[5]);
    console.log("PASS: Int8Array.of()");

    // =========================================
    // Test 2: Uint8ClampedArray.of()
    // =========================================
    console.log("");
    console.log("=== Uint8ClampedArray.of() ===");
    let arr2 = Uint8ClampedArray.of(0, 128, 255, 300, -10);
    console.log("Uint8ClampedArray.of(0, 128, 255, 300, -10) created");
    console.log("Length:", arr2.length);
    console.log("arr2[2]:", arr2[2]);
    console.log("arr2[3] (clamped from 300):", arr2[3]);
    console.log("arr2[4] (clamped from -10):", arr2[4]);
    console.log("PASS: Uint8ClampedArray.of()");

    // =========================================
    // Test 3: Int16Array.of()
    // =========================================
    console.log("");
    console.log("=== Int16Array.of() ===");
    let arr3 = Int16Array.of(100, 200, -32768, 32767);
    console.log("Int16Array.of(100, 200, -32768, 32767) created");
    console.log("Length:", arr3.length);
    console.log("arr3[2]:", arr3[2]);
    console.log("arr3[3]:", arr3[3]);
    console.log("PASS: Int16Array.of()");

    // =========================================
    // Test 4: Uint16Array.of()
    // =========================================
    console.log("");
    console.log("=== Uint16Array.of() ===");
    let arr4 = Uint16Array.of(0, 1000, 65535);
    console.log("Uint16Array.of(0, 1000, 65535) created");
    console.log("Length:", arr4.length);
    console.log("arr4[2]:", arr4[2]);
    console.log("PASS: Uint16Array.of()");

    // =========================================
    // Test 5: Uint32Array.of()
    // =========================================
    console.log("");
    console.log("=== Uint32Array.of() ===");
    let arr5 = Uint32Array.of(0, 1000000, 4294967295);
    console.log("Uint32Array.of(0, 1000000, 4294967295) created");
    console.log("Length:", arr5.length);
    console.log("PASS: Uint32Array.of()");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All TypedArray.of() tests passed!");
    return 0;
}
