// Simple test to understand how Nova handles doubles
console.log("Testing Math functions with doubles:");
console.log("");

// Test Math.sqrt() - known to work
console.log("Test Math.sqrt():");
var sqrt4 = Math.sqrt(4);
console.log("  Math.sqrt(4) = " + sqrt4);
var sqrt9 = Math.sqrt(9);
console.log("  Math.sqrt(9) = " + sqrt9);
console.log("");

// Test Math.pow() - known to work
console.log("Test Math.pow():");
var pow2_3 = Math.pow(2, 3);
console.log("  Math.pow(2, 3) = " + pow2_3);
console.log("");

// Test Math.random()
console.log("Test Math.random():");
var r1 = Math.random();
console.log("  Math.random() = " + r1);
var r2 = Math.random();
console.log("  Math.random() = " + r2);
var r3 = Math.random();
console.log("  Math.random() = " + r3);
console.log("");

// Test using Math.random() in calculation
console.log("Test Math.random() in calculation:");
var scaled = Math.random() * 100;
console.log("  Math.random() * 100 = " + scaled);
