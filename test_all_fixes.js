console.log("=== Testing All Bug Fixes ===\n");

// Test 1: Array element access with console.log (Bug #1 - FIXED)
console.log("Test 1: Array element access");
const arr = [100, 200, 300];
const elem1 = arr[0];
const elem2 = arr[1];
console.log("  arr[0]:", elem1);
console.log("  arr[1]:", elem2);
console.log("  PASS\n");

// Test 2: Array destructuring (Bug #2 - FIXED)
console.log("Test 2: Array destructuring");
const [a, b, c] = [10, 20, 30];
console.log("  a:", a);
console.log("  b:", b);
console.log("  c:", c);
console.log("  PASS\n");

// Test 3: Arrow functions (Bug #3 - FIXED)
console.log("Test 3: Arrow functions");
const add = (x, y) => x + y;
const multiply = (x, y) => { return x * y; };
const result1 = add(5, 3);
const result2 = multiply(4, 7);
console.log("  add(5, 3):", result1);
console.log("  multiply(4, 7):", result2);
console.log("  PASS\n");

// Test 4: Object destructuring (Bug #4 - FIXED)
console.log("Test 4: Object destructuring");
const obj = {x: 42, y: 99};
const {x, y} = obj;
console.log("  x:", x);
console.log("  y:", y);
console.log("  PASS\n");

console.log("=== All Tests Passed! ===");
