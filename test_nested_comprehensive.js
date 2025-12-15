// Comprehensive nested call and template literal tests
console.log("=== Nested Function Call Tests ===");

// Test 1: Basic nested call
function add(x) { return x + 10; }
function multiply(x) { return x * 2; }
console.log("Test 1:", multiply(add(5)));  // Should be 30

// Test 2: Triple nesting
function a() { return 5; }
function b(x) { return x + 3; }
function c(x) { return x * 2; }
console.log("Test 2:", c(b(a())));  // Should be 16

// Test 3: Template literals with strings
const name = "Alice";
const greeting = `Hello, ${name}!`;
console.log("Test 3:", greeting);

// Test 4: Template with multiple interpolations
const first = "John";
const last = "Doe";
const full = `${first} ${last}`;
console.log("Test 4:", full);

// Test 5: Nested calls in expressions
const result = multiply(add(3)) + multiply(add(2));
console.log("Test 5:", result);  // (3+10)*2 + (2+10)*2 = 26 + 24 = 50

console.log("\n=== All Tests Complete ===");
