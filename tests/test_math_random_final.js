// Final test for Math.random()
console.log("=== Math.random() Implementation Test ===");
console.log("");

// Test 1: Direct calls
console.log("Test 1: Direct calls to Math.random()");
console.log(Math.random());
console.log(Math.random());
console.log(Math.random());
console.log("");

// Test 2: Stored in variables
console.log("Test 2: Stored in variables");
var r1 = Math.random();
var r2 = Math.random();
var r3 = Math.random();
console.log(r1);
console.log(r2);
console.log(r3);
console.log("");

// Test 3: Used in expressions
console.log("Test 3: Used in Math.floor() expression");
console.log(Math.floor(Math.random() * 10));
console.log(Math.floor(Math.random() * 10));
console.log(Math.floor(Math.random() * 10));
console.log("");

console.log("All tests completed successfully!");
