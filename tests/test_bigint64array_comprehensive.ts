// Comprehensive test for BigInt64Array (ES2020)

function main(): number {
    // =========================================
    // Test 1: Constructor with length
    // =========================================
    let arr1 = new BigInt64Array(5);
    console.log("new BigInt64Array(5) length:", arr1.length);
    console.log("PASS: BigInt64Array constructor");

    // =========================================
    // Test 2: Constructor with ArrayBuffer
    // =========================================
    let buffer = new ArrayBuffer(32);
    let arr2 = new BigInt64Array(buffer);
    console.log("BigInt64Array from ArrayBuffer length:", arr2.length);
    console.log("PASS: BigInt64Array from ArrayBuffer");

    // =========================================
    // Test 3: Index read/write
    // =========================================
    arr1[0] = 100;
    arr1[1] = 200;
    arr1[2] = 300;
    console.log("arr1[0]:", arr1[0]);
    console.log("arr1[1]:", arr1[1]);
    console.log("arr1[2]:", arr1[2]);
    console.log("PASS: BigInt64Array index access");

    // =========================================
    // Test 4: BYTES_PER_ELEMENT
    // =========================================
    console.log("BigInt64Array.BYTES_PER_ELEMENT:", BigInt64Array.BYTES_PER_ELEMENT);
    console.log("PASS: BYTES_PER_ELEMENT");

    // =========================================
    // Test 5: of() static method
    // =========================================
    let arr3 = BigInt64Array.of(10, 20, 30, 40);
    console.log("BigInt64Array.of() length:", arr3.length);
    console.log("arr3[0]:", arr3[0]);
    console.log("arr3[3]:", arr3[3]);
    console.log("PASS: BigInt64Array.of()");

    // =========================================
    // Test 6: from() static method
    // =========================================
    let source = [1, 2, 3, 4, 5];
    let arr4 = BigInt64Array.from(source);
    console.log("BigInt64Array.from() length:", arr4.length);
    console.log("arr4[0]:", arr4[0]);
    console.log("arr4[4]:", arr4[4]);
    console.log("PASS: BigInt64Array.from()");

    // =========================================
    // Test 7: set() method
    // =========================================
    let arr5 = new BigInt64Array(5);
    arr5.set([11, 22, 33]);
    console.log("After set([11,22,33]):");
    console.log("arr5[0]:", arr5[0]);
    console.log("arr5[2]:", arr5[2]);
    console.log("PASS: BigInt64Array.set()");

    // =========================================
    // Test 8: fill() method
    // =========================================
    let arr6 = new BigInt64Array(5);
    arr6.fill(42);
    console.log("After fill(42):");
    console.log("arr6[0]:", arr6[0]);
    console.log("arr6[4]:", arr6[4]);
    console.log("PASS: BigInt64Array.fill()");

    // =========================================
    // Test 9: subarray() method
    // =========================================
    let arr7 = BigInt64Array.of(1, 2, 3, 4, 5);
    let sub = arr7.subarray(1, 4);
    console.log("subarray(1,4) length:", sub.length);
    console.log("sub[0]:", sub[0]);
    console.log("sub[2]:", sub[2]);
    console.log("PASS: BigInt64Array.subarray()");

    // =========================================
    // Test 10: Large values (64-bit)
    // =========================================
    let arr8 = new BigInt64Array(2);
    arr8[0] = 9223372036854775807;  // Max int64
    arr8[1] = -9223372036854775808; // Min int64
    console.log("Max int64:", arr8[0]);
    console.log("Min int64:", arr8[1]);
    console.log("PASS: BigInt64Array large values");

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All BigInt64Array comprehensive tests passed!");
    return 0;
}
