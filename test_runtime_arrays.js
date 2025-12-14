// Test: Array Runtime Functions
console.log("=== ARRAY RUNTIME TEST ===");

// 1. Basic array
const arr = [1, 2, 3];
console.log("Array[0]:", arr[0]);
console.log("Array[1]:", arr[1]);
console.log("Array[2]:", arr[2]);

// 2. Array length
console.log("Length:", arr.length);

// 3. Array push
arr.push(4);
console.log("After push, length:", arr.length);
console.log("Element [3]:", arr[3]);

// 4. Array map
const doubled = arr.map(x => x * 2);
console.log("Doubled[0]:", doubled[0]);
console.log("Doubled[1]:", doubled[1]);

// 5. Array filter
const evens = arr.filter(x => x % 2 === 0);
console.log("Evens[0]:", evens[0]);

console.log("=== ARRAYS WORK ===");
