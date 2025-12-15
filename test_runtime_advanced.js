console.log("=== ADVANCED RUNTIME TEST ===");

// Test 1: Array.map
const nums = [1, 2, 3];
const doubled = nums.map(x => x * 2);
console.log("1. Array.map:", doubled[0], doubled[1], doubled[2]);

// Test 2: Array.filter
const values = [1, 2, 3, 4, 5];
const evens = values.filter(x => x % 2 === 0);
console.log("2. Array.filter:", evens.length);

// Test 3: Array.reduce
const numbers = [1, 2, 3, 4];
const sum = numbers.reduce((acc, x) => acc + x, 0);
console.log("3. Array.reduce:", sum);

// Test 4: String methods
const text = "hello";
const upper = text.toUpperCase();
const lower = "WORLD".toLowerCase();
console.log("4. String methods:", upper, lower);

// Test 5: String.slice
const str = "JavaScript";
const sliced = str.slice(0, 4);
console.log("5. String.slice:", sliced);

console.log("=== ADVANCED COMPLETE ===");
