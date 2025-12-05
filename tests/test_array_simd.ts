// Test SIMD-optimized array operations
console.log("=== Testing SIMD Array Operations ===");

// Test 1: indexOf with large array (SIMD kicks in at length >= 8)
console.log("\nTest 1: indexOf with SIMD (16 elements)");
let arr1 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];
console.log("Finding 8:");
console.log(arr1.indexOf(8));
console.log("Finding 15:");
console.log(arr1.indexOf(15));
console.log("Finding 99 (not found):");
console.log(arr1.indexOf(99));

// Test 2: includes with large array
console.log("\nTest 2: includes with SIMD");
let arr2 = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100];
console.log("Has 50:");
console.log(arr2.includes(50));
console.log("Has 999:");
console.log(arr2.includes(999));

// Test 3: fill with SIMD (kicks in at length >= 16)
console.log("\nTest 3: fill with SIMD (20 elements)");
let arr3 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20];
arr3.fill(42);
console.log("After fill(42):");
console.log("First:");
console.log(arr3[0]);
console.log("Middle:");
console.log(arr3[10]);
console.log("Last:");
console.log(arr3[19]);

// Test 4: Large array capacity growth
console.log("\nTest 4: Capacity growth (1000 elements)");
let arr4 = [];
for (let i = 0; i < 1000; i++) {
    arr4.push(i);
}
console.log("Pushed 1000 elements");
console.log("arr[0]:");
console.log(arr4[0]);
console.log("arr[500]:");
console.log(arr4[500]);
console.log("arr[999]:");
console.log(arr4[999]);
console.log("indexOf(777):");
console.log(arr4.indexOf(777));

console.log("\n✓ All SIMD tests completed!");
