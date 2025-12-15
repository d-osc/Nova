// Test spread operator fix
console.log("=== TESTING SPREAD FIX ===");

const arr1 = [1, 2, 3];
console.log("Original array:");
console.log(arr1);

const arr2 = [...arr1, 4, 5];
console.log("Spread array:");
console.log(arr2);

console.log("Array elements:");
console.log("arr2[0]:", arr2[0]);
console.log("arr2[4]:", arr2[4]);

console.log("Array length:");
console.log(arr2.length);

console.log("=== TEST COMPLETE ===");
