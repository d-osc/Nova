console.log("=== Testing Destructuring ===\n");

// Test 1: Array destructuring
console.log("Test 1: Array destructuring");
const arr = [1, 2, 3];
const [a, b, c] = arr;
console.log("  a:", a);
console.log("  b:", b);
console.log("  c:", c);
console.log("  PASS\n");

// Test 2: Object destructuring
console.log("Test 2: Object destructuring");
const obj = { x: 10, y: 20 };
const { x, y } = obj;
console.log("  x:", x);
console.log("  y:", y);
console.log("  PASS\n");

console.log("=== Destructuring Tests Complete ===");
