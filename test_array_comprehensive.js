// Comprehensive array operations test
console.log("=== Array Length Tests ===");

// Test 1: Simple array length
const arr1 = [1, 2, 3, 4, 5];
console.log("arr1.length:", arr1.length);

// Test 2: Empty array
const arr2 = [];
console.log("arr2.length:", arr2.length);

// Test 3: Single element
const arr3 = [42];
console.log("arr3.length:", arr3.length);

// Test 4: Mixed types
const arr4 = [1, "hello", 3.14];
console.log("arr4.length:", arr4.length);

// Test 5: Length in expressions
const arr5 = [10, 20, 30];
const doubleLen = arr5.length * 2;
console.log("arr5.length * 2:", doubleLen);

// Test 6: Length comparison
const arr6 = [1, 2];
const arr7 = [1, 2, 3];
if (arr7.length > arr6.length) {
    console.log("arr7 is longer than arr6");
}

console.log("\n=== All Array Tests Passed ===");
