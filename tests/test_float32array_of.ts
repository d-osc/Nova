// Test Float32Array.of() static method (ES2015)

function main(): number {
    // =========================================
    // Test 1: Float32Array.of() with values
    // =========================================
    let arr1 = Float32Array.of(1, 2, 3, 4, 5);
    console.log("Float32Array.of(1, 2, 3, 4, 5) created");
    console.log("Length:", arr1.length);
    console.log("PASS: Float32Array.of() basic");

    // =========================================
    // Test 2: Access elements
    // =========================================
    console.log("arr1[0]:", arr1[0]);
    console.log("arr1[1]:", arr1[1]);
    console.log("arr1[2]:", arr1[2]);
    console.log("arr1[3]:", arr1[3]);
    console.log("arr1[4]:", arr1[4]);
    console.log("PASS: Element access");

    // =========================================
    // Test 3: Float32Array.of() with single value
    // =========================================
    let arr2 = Float32Array.of(42);
    console.log("Float32Array.of(42) length:", arr2.length);
    console.log("arr2[0]:", arr2[0]);
    console.log("PASS: Float32Array.of() single value");

    // =========================================
    // Test 4: Float32Array.of() with no values
    // =========================================
    let arr3 = Float32Array.of();
    console.log("Float32Array.of() length:", arr3.length);
    console.log("PASS: Float32Array.of() empty");

    // =========================================
    // Test 5: Float32Array.of() with 8 values (max)
    // =========================================
    let arr4 = Float32Array.of(10, 20, 30, 40, 50, 60, 70, 80);
    console.log("Float32Array.of(10-80) length:", arr4.length);
    console.log("arr4[7]:", arr4[7]);
    console.log("PASS: Float32Array.of() 8 values");

    // =========================================
    // Test 6: Verify BYTES_PER_ELEMENT
    // =========================================
    console.log("BYTES_PER_ELEMENT:", arr1.BYTES_PER_ELEMENT);
    console.log("PASS: BYTES_PER_ELEMENT");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All Float32Array.of() tests passed!");
    return 0;
}
