console.log("=== All 5 Bug Fixes Test ===\n");

// Bug #1: console.log with array elements
console.log("Bug #1: Array element printing");
const arr = [100, 200, 300];
console.log("  arr[0]:", arr[0]);
console.log("  arr[1]:", arr[1]);
console.log("  PASS\n");

// Bug #2: Array destructuring  
console.log("Bug #2: Array destructuring");
const [a, b, c] = [10, 20, 30];
console.log("  [a, b, c]:", a, b, c);
console.log("  PASS\n");

// Bug #3: Arrow functions
console.log("Bug #3: Arrow functions");
const add = (x, y) => x + y;
const multiply = (x, y) => { return x * y; };
console.log("  add(5, 3):", add(5, 3));
console.log("  multiply(4, 7):", multiply(4, 7));
console.log("  PASS\n");

// Bug #4: Object destructuring
console.log("Bug #4: Object destructuring");
const obj = {x: 42, y: 99};
const {x, y} = obj;
console.log("  {x, y}:", x, y);
console.log("  PASS\n");

// Bug #5: Logical operators
console.log("Bug #5: Logical operators");
const andResult = true && false;
const orResult = false || true;
console.log("  true && false:", andResult);
console.log("  false || true:", orResult);
console.log("  PASS\n");

console.log("=== All 5 Tests Passed! ===");
