// Test ultra-optimized Array operations

console.log("=== Array Ultra Optimization Test ===");

// Test 1: Push/Pop (Fast path optimization)
console.log("\n1. Testing push/pop (fast path):");
let arr1 = [];
for (let i = 0; i < 10; i++) {
    arr1.push(i);
}
console.log("Pushed 10 elements:", arr1.length);
let popped = arr1.pop();
console.log("Popped:", popped, "Length:", arr1.length);

// Test 2: indexOf (SIMD optimization)
console.log("\n2. Testing indexOf (SIMD):");
let arr2 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
let index1 = arr2.indexOf(5);
let index2 = arr2.indexOf(99);
console.log("indexOf(5):", index1, "(expected: 4)");
console.log("indexOf(99):", index2, "(expected: -1)");

// Test 3: includes (SIMD optimization)
console.log("\n3. Testing includes (SIMD):");
let arr3 = [10, 20, 30, 40, 50];
let has1 = arr3.includes(30);
let has2 = arr3.includes(99);
console.log("includes(30):", has1, "(expected: 1/true)");
console.log("includes(99):", has2, "(expected: 0/false)");

// Test 4: fill (SIMD optimization)
console.log("\n4. Testing fill (SIMD):");
let arr4 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
arr4.fill(42);
console.log("After fill(42):", arr4[0], arr4[5], arr4[9], "(all should be 42)");

// Test 5: Large array (Capacity growth optimization)
console.log("\n5. Testing large array (capacity growth):");
let arr5 = [];
for (let i = 0; i < 1000; i++) {
    arr5.push(i);
}
console.log("Pushed 1000 elements, length:", arr5.length);
console.log("First:", arr5[0], "Middle:", arr5[500], "Last:", arr5[999]);

// Test 6: Array methods
console.log("\n6. Testing array methods:");
let arr6 = [1, 2, 3, 4, 5];
console.log("Sum via loop:");
let sum = 0;
for (let i = 0; i < arr6.length; i++) {
    sum = sum + arr6[i];
}
console.log("Sum of [1,2,3,4,5]:", sum, "(expected: 15)");

console.log("\n✓ All tests completed!");
