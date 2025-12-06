// Simple Array Benchmark (without Date.now)
console.log("Array Benchmark - Simple Version");
console.log("=================================");

const size = 10000;

console.log("Creating array with " + size + " elements...");
const arr = [];
for (let i = 0; i < size; i++) {
    arr.push(i);
}
console.log("Array created! Length: " + arr.length);

console.log("\nCalculating sum...");
let sum = 0;
for (let i = 0; i < size; i++) {
    sum = sum + i;
}
console.log("Sum: " + sum);

const expected = (size * (size - 1)) / 2;
console.log("Expected: " + expected);

console.log("\nMapping array (multiply by 2)...");
const mapped = arr.map(x => x * 2);
console.log("Mapped array length: " + mapped.length);

console.log("\n=== Benchmark Complete ===");
console.log("All operations completed successfully!");
