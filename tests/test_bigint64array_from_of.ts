// Test BigInt64Array.from() and BigInt64Array.of() static methods

function main(): number {
    // =========================================
    // Test 1: BigInt64Array.of() with values
    // =========================================
    let arr1 = BigInt64Array.of(1, 2, 3, 4, 5);
    console.log("BigInt64Array.of(1,2,3,4,5) created");
    console.log("Length:", arr1.length);
    console.log("arr1[0]:", arr1[0]);
    console.log("arr1[4]:", arr1[4]);

    // =========================================
    // Test 2: BigUint64Array.of() with values
    // =========================================
    let arr2 = BigUint64Array.of(10, 20, 30);
    console.log("");
    console.log("BigUint64Array.of(10,20,30) created");
    console.log("Length:", arr2.length);
    console.log("arr2[0]:", arr2[0]);
    console.log("arr2[2]:", arr2[2]);

    // =========================================
    // Test 3: BigInt64Array.from() from array
    // =========================================
    let source = [100, 200, 300, 400];
    let arr3 = BigInt64Array.from(source);
    console.log("");
    console.log("BigInt64Array.from([100,200,300,400]) created");
    console.log("Length:", arr3.length);
    console.log("arr3[0]:", arr3[0]);
    console.log("arr3[3]:", arr3[3]);

    // =========================================
    // Test 4: BigUint64Array.from() from array
    // =========================================
    let source2 = [1000, 2000, 3000];
    let arr4 = BigUint64Array.from(source2);
    console.log("");
    console.log("BigUint64Array.from([1000,2000,3000]) created");
    console.log("Length:", arr4.length);
    console.log("arr4[0]:", arr4[0]);
    console.log("arr4[2]:", arr4[2]);

    // =========================================
    // All tests passed
    // =========================================
    console.log("");
    console.log("All BigInt64Array/BigUint64Array from/of tests passed!");
    return 0;
}
