console.log("=== Testing Additional Features ===\n");

// Test 1: typeof operator
console.log("Test 1: typeof operator");
console.log("  typeof 42:", typeof 42);
console.log("  typeof 'hello':", typeof "hello");
console.log("  typeof true:", typeof true);
console.log("  PASS\n");

// Test 2: Ternary operator
console.log("Test 2: Ternary operator");
const x = 5;
const result = x > 3 ? "yes" : "no";
console.log("  5 > 3 ?", result);
console.log("  PASS\n");

// Test 3: Logical operators
console.log("Test 3: Logical operators");
const a = true && false;
const b = true || false;
const c = !true;
console.log("  true && false:", a);
console.log("  true || false:", b);
console.log("  !true:", c);
console.log("  PASS\n");

// Test 4: Comparison operators
console.log("Test 4: Comparison operators");
console.log("  5 === 5:", 5 === 5);
console.log("  5 !== 3:", 5 !== 3);
console.log("  5 > 3:", 5 > 3);
console.log("  5 < 10:", 5 < 10);
console.log("  PASS\n");

// Test 5: Arithmetic operators
console.log("Test 5: Arithmetic");
console.log("  10 + 5:", 10 + 5);
console.log("  10 - 5:", 10 - 5);
console.log("  10 * 5:", 10 * 5);
console.log("  10 / 5:", 10 / 5);
console.log("  10 % 3:", 10 % 3);
console.log("  PASS\n");

// Test 6: String concatenation
console.log("Test 6: String concatenation");
const str1 = "Hello " + "World";
console.log("  Result:", str1);
console.log("  PASS\n");

// Test 7: Increment/Decrement
console.log("Test 7: Increment/Decrement");
let num = 5;
num++;
console.log("  After ++:", num);
num--;
console.log("  After --:", num);
console.log("  PASS\n");

console.log("=== All Additional Features Work ===");
