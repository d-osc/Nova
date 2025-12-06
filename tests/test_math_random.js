// Test Math.random() implementation
console.log("Testing Math.random():");
console.log("");

// Test 1: Basic functionality
console.log("Test 1: Generate 10 random numbers");
for (var i = 0; i < 10; i++) {
    var r = Math.random();
    console.log("  Random " + i + ": " + r);
}
console.log("");

// Test 2: Verify range (should be between 0 and 1)
console.log("Test 2: Verify range over 100 values");
var allInRange = true;
var min = 1.0;
var max = 0.0;
for (var i = 0; i < 100; i++) {
    var r = Math.random();
    if (r < 0 || r >= 1) {
        allInRange = false;
        console.log("  ERROR: Value out of range: " + r);
    }
    if (r < min) min = r;
    if (r > max) max = r;
}
if (allInRange) {
    console.log("  PASS: All values in range [0, 1)");
    console.log("  Min: " + min);
    console.log("  Max: " + max);
} else {
    console.log("  FAIL: Some values out of range");
}
console.log("");

// Test 3: Use in expressions
console.log("Test 3: Use in expressions");
var randomInt = Math.floor(Math.random() * 100);
console.log("  Random integer 0-99: " + randomInt);
console.log("");

console.log("All tests completed!");
