// Test ternary operator fix
console.log("Testing ternary operator...");

// Test 1: Basic ternary with strings
const result1 = 5 > 3 ? "yes" : "no";
console.log("Test 1:", result1);  // Should print: yes

// Test 2: Ternary with numbers
const result2 = 10 < 5 ? 100 : 200;
console.log("Test 2:", result2);  // Should print: 200

// Test 3: Ternary with boolean
const result3 = true ? "correct" : "wrong";
console.log("Test 3:", result3);  // Should print: correct

// Test 4: Nested ternary
const x = 15;
const result4 = x > 20 ? "big" : x > 10 ? "medium" : "small";
console.log("Test 4:", result4);  // Should print: medium

// Test 5: Ternary in expression
const a = 3;
const b = 7;
const max = a > b ? a : b;
console.log("Test 5: max of", a, "and", b, "is", max);  // Should print: 7

console.log("All ternary tests completed!");
