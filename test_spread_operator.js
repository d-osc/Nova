// Test Spread operator in arrays
const arr1 = [1, 2, 3];
const arr2 = [...arr1, 4, 5];
console.log("arr1:");
console.log(arr1);
console.log("arr2 (spread):");
console.log(arr2);

// Test spread with multiple arrays
const arr3 = [10, 20];
const combined = [...arr1, ...arr3];
console.log("combined:");
console.log(combined);
