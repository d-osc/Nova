// Simple Array Benchmark (without Date.now())
console.log("Array Benchmark Test");
console.log("====================");

const size = 1000;

// Test 1: Array creation with push
console.log("\nTest 1: Creating array with " + size + " elements");
const arr = [];
for (let i = 0; i < size; i++) {
    arr.push(i);
}
console.log("Array created! Length: " + arr.length);

// Test 2: Calculate sum
console.log("\nTest 2: Calculating sum of array elements");
let sum = 0;
for (let i = 0; i < arr.length; i++) {
    sum = sum + i;
}
console.log("Sum: " + sum);
console.log("Expected: 499500");

// Test 3: Array map
console.log("\nTest 3: Mapping array (multiply by 2)");
const mapped = arr.map(x => x * 2);
console.log("Mapped array length: " + mapped.length);
console.log("First element: " + mapped[0]);
console.log("Last element: " + mapped[size - 1]);
console.log("Expected last: " + ((size - 1) * 2));

console.log("\n=== Benchmark Complete ===");
