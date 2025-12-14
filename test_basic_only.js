// Test basic features only
console.log("=== Testing Basic Features ===");

// 1. Variables
const x = 42;
let y = 100;
console.log("✓ Variables:", x, y);

// 2. Arrays
const arr = [1, 2, 3];
console.log("✓ Arrays:", arr[0], arr[1], arr[2]);

// 3. Array push
arr.push(4);
console.log("✓ Array.push:", arr[3]);

// 4. Spread
const arr2 = [...arr, 5, 6];
console.log("✓ Spread:", arr2[0], arr2[4], arr2[5]);

// 5. Objects
const obj = { a: 1, b: 2 };
console.log("✓ Objects:", obj.a, obj.b);

console.log("=== Basic tests passed ===");
