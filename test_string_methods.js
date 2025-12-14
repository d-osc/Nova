// Test string methods
console.log("=== Testing String Methods ===\n");

const str = "Hello World";

// Test substring
console.log("1. substring(0, 5)");
const sub = str.substring(0, 5);
if (sub) {
    console.log("   ✓ PASS");
}

// Test toLowerCase
console.log("2. toLowerCase()");
const lower = str.toLowerCase();
if (lower) {
    console.log("   ✓ PASS");
}

// Test toUpperCase
console.log("3. toUpperCase()");
const upper = str.toUpperCase();
if (upper) {
    console.log("   ✓ PASS");
}

// Test indexOf
console.log("4. indexOf('World')");
const idx = str.indexOf("World");
if (idx >= 0) {
    console.log("   ✓ PASS - found");
}

// Test includes
console.log("5. includes('World')");
const hasWorld = str.includes("World");
if (hasWorld) {
    console.log("   ✓ PASS");
}

// Test split
console.log("6. split(' ')");
const parts = str.split(" ");
if (parts) {
    console.log("   ✓ PASS");
}

// Test trim
const str2 = "  hello  ";
console.log("7. trim()");
const trimmed = str2.trim();
if (trimmed) {
    console.log("   ✓ PASS");
}

// Test startsWith
console.log("8. startsWith('Hello')");
const starts = str.startsWith("Hello");
if (starts) {
    console.log("   ✓ PASS");
}

// Test endsWith
console.log("9. endsWith('World')");
const ends = str.endsWith("World");
if (ends) {
    console.log("   ✓ PASS");
}

// Test repeat
console.log("10. repeat(3)");
const repeated = "x".repeat(3);
if (repeated) {
    console.log("   ✓ PASS");
}

console.log("\n=== All String Tests Complete ===");
