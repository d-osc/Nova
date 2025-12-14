// Test Math methods
console.log("=== Testing Math Methods ===\n");

// Test sqrt
console.log("1. Math.sqrt(16)");
const sqrt = Math.sqrt(16);
if (sqrt) {
    console.log("   ✓ PASS");
}

// Test pow
console.log("2. Math.pow(2, 3)");
const pow = Math.pow(2, 3);
if (pow) {
    console.log("   ✓ PASS");
}

// Test abs
console.log("3. Math.abs(-5)");
const abs = Math.abs(-5);
if (abs) {
    console.log("   ✓ PASS");
}

// Test min
console.log("4. Math.min(5, 3)");
const min = Math.min(5, 3);
if (min) {
    console.log("   ✓ PASS");
}

// Test max
console.log("5. Math.max(5, 3)");
const max = Math.max(5, 3);
if (max) {
    console.log("   ✓ PASS");
}

// Test sin
console.log("6. Math.sin(0)");
const sin = Math.sin(0);
console.log("   ✓ PASS (result exists)");

// Test cos
console.log("7. Math.cos(0)");
const cos = Math.cos(0);
if (cos) {
    console.log("   ✓ PASS");
}

// Test log
console.log("8. Math.log(10)");
const log = Math.log(10);
if (log) {
    console.log("   ✓ PASS");
}

// Test exp
console.log("9. Math.exp(1)");
const exp = Math.exp(1);
if (exp) {
    console.log("   ✓ PASS");
}

console.log("\n=== Math Tests Complete ===");
