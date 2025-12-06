// Test array.map() only
console.log("Creating array with literal...");
const arr = [1, 2, 3, 4, 5];
console.log("Array created with " + arr.length + " elements");

console.log("Starting map operation...");
const mapped = arr.map(x => x * 2);
console.log("Map completed!");

console.log("Mapped array length: " + mapped.length);
console.log("Done!");
