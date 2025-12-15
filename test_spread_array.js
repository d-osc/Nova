// Test array spread
const arr1 = [1, 2, 3];
const arr2 = [4, 5, 6];

// Simple spread
const combined = [...arr1, ...arr2];

console.log("arr1:", arr1[0], arr1[1], arr1[2]);
console.log("arr2:", arr2[0], arr2[1], arr2[2]);
console.log("combined:", combined[0], combined[1], combined[2], combined[3], combined[4], combined[5]);

console.log("\nExpected:");
console.log("arr1: 1 2 3");
console.log("arr2: 4 5 6");
console.log("combined: 1 2 3 4 5 6");
