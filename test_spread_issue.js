// Test spread array .length issue
const arr1 = [1, 2, 3];
console.log("Original array length:", arr1.length);

const arr2 = [...arr1, 4, 5];
console.log("Spread array:", arr2);
console.log("Spread array length:", arr2.length);
