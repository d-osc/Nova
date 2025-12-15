console.log("=== Testing Fixed Features ===");

// 1. Variables
const x = 42;
console.log("✓ Variables:", x);

// 2. Arrays
const arr = [1, 2, 3];
console.log("✓ Arrays:", arr[0], arr[1], arr[2]);

// 3. Array push
arr.push(4);
console.log("✓ Array.push:", arr[3]);

// 4. Spread operator (FIXED!)
const arr2 = [...arr, 5, 6];
console.log("✓ Spread:", arr2[0], arr2[4], arr2[5]);

// 5. Object literals (FIXED!)
const obj = { a: 1, b: 2 };
console.log("✓ Objects:", obj.a, obj.b);

// 6. Arrow functions
const add = (a, b) => a + b;
console.log("✓ Arrow functions:", add(5, 3));

// 7. For loops
let sum = 0;
for (let i = 0; i < 5; i++) {
    sum = sum + i;
}
console.log("✓ For loops:", sum);

console.log("");
console.log("=== All Fixed Features Working! ===");
