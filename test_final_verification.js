// Final verification of all bug fixes

console.log("=== Test 1: Integer Overflow Fix ===");
const size = 1000;
let sum = 0;
for (let i = 0; i < size; i++) {
    sum = sum + i;
}
console.log("Sum of 0 to 999: " + sum);
console.log("Expected: 499500");

console.log("\n=== Test 2: String Coercion ===");
console.log("Number: " + 42);
console.log("Boolean: " + true);
console.log("Large number: " + 123456789012345);

console.log("\n=== Test 3: Array.length Property ===");
const arr = [];
arr.push(10);
arr.push(20);
arr.push(30);
console.log("Array length after 3 pushes: " + arr.length);
console.log("Expected: 3");

console.log("\n=== All tests complete! ===");
