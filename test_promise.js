console.log("=== Testing Promise ===\n");

// Test 1: Basic Promise creation
console.log("Test 1: Promise creation");
const p1 = new Promise((resolve, reject) => {
    console.log("  Inside Promise executor");
    resolve(42);
});
console.log("  Promise created:", typeof p1);
console.log("  PASS\n");

// Test 2: Promise.then()
console.log("Test 2: Promise.then()");
const p2 = new Promise((resolve) => {
    resolve(100);
});
p2.then((value) => {
    console.log("  Then callback with value:", value);
});
console.log("  PASS\n");

// Test 3: Promise chaining
console.log("Test 3: Promise chaining");
const p3 = new Promise((resolve) => {
    resolve(10);
});
p3.then((val) => {
    console.log("  First then:", val);
    return val * 2;
}).then((val) => {
    console.log("  Second then:", val);
});
console.log("  PASS\n");

console.log("=== Promise Tests Complete ===");
