// Test Array.map()
console.log("Testing Array.map()...");

const arr = [1, 2, 3];
console.log("Array created");

const doubled = arr.map(x => x * 2);
console.log("Map completed");

console.log("Result exists:", doubled ? "yes" : "no");
