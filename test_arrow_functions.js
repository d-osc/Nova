console.log("=== Testing Arrow Functions ===\n");

// Test 1: Basic arrow function
console.log("Test 1: Basic arrow function");
const add = (a, b) => a + b;
console.log("  add(5, 3):", add(5, 3));
console.log("  PASS\n");

// Test 2: Arrow function with block
console.log("Test 2: Arrow with block");
const multiply = (a, b) => {
    const result = a * b;
    return result;
};
console.log("  multiply(4, 5):", multiply(4, 5));
console.log("  PASS\n");

// Test 3: Arrow function in array methods
console.log("Test 3: Arrow in map");
const numbers = [1, 2, 3];
const doubled = numbers.map(n => n * 2);
console.log("  doubled:", doubled);
console.log("  PASS\n");

console.log("=== Arrow Function Tests Complete ===");
