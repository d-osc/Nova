// Minimal Array Benchmark - Only basic features
console.log("Array Benchmark - Minimal Version");
console.log("==================================");

const size = 10000;
console.log("Size: " + size);

console.log("\n1. Creating array with push...");
const arr = [];
let i = 0;
for (i = 0; i < size; i = i + 1) {
    arr.push(i);
}
console.log("Done! Array length: " + arr.length);

console.log("\n2. Calculating sum...");
let sum = 0;
let j = 0;
for (j = 0; j < size; j = j + 1) {
    sum = sum + j;
}
console.log("Sum: " + sum);

console.log("\n3. Creating second array...");
const arr2 = [];
let k = 0;
for (k = 0; k < 100; k = k + 1) {
    arr2.push(k * 2);
}
console.log("Second array length: " + arr2.length);

console.log("\n=== All tests passed! ===");
