console.log("Testing for-in loop fix:");
console.log();

// Test 1: For-in with object
console.log("Test 1: Object iteration");
const obj = { name: "Alice", age: 30, city: "NYC" };

for (const key in obj) {
    console.log("Key:", key);
}

console.log();

// Test 2: For-in with array (should print indices as strings)
console.log("Test 2: Array iteration");
const arr = [10, 20, 30];

for (const key in arr) {
    console.log("Index:", key);
}

console.log();
console.log("Test complete!");
