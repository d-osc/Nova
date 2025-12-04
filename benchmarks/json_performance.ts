// JSON Performance Test

const iterations = 1000;

console.log("JSON Stringify Performance Test");
console.log("Iterations: 1000");

// Test 1: Number stringify
for (let i = 0; i < iterations; i++) {
    const result = JSON.stringify(42);
}
console.log("Numbers: Complete");

// Test 2: String stringify
for (let i = 0; i < iterations; i++) {
    const result = JSON.stringify("Hello World");
}
console.log("Strings: Complete");

// Test 3: Boolean stringify
for (let i = 0; i < iterations; i++) {
    const result = JSON.stringify(true);
}
console.log("Booleans: Complete");

// Test 4: Array stringify
for (let i = 0; i < iterations; i++) {
    const arr = [1, 2, 3, 4, 5];
    const result = JSON.stringify(arr);
}
console.log("Arrays: Complete");

console.log("All performance tests completed!");
