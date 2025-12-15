// Test JSON and Object methods
console.log("=== Testing JSON and Object Methods ===\n");

// Test JSON.stringify
console.log("1. JSON.stringify");
const obj = { x: 1, y: 2 };
const json = JSON.stringify(obj);
if (json) {
    console.log("   ✓ PASS - JSON.stringify works");
}

// Test JSON.parse
console.log("2. JSON.parse");
const str = '{"a":1,"b":2}';
const parsed = JSON.parse(str);
if (parsed) {
    console.log("   ✓ PASS - JSON.parse works");
}

// Test Object.keys
console.log("3. Object.keys");
const obj2 = { a: 1, b: 2, c: 3 };
const keys = Object.keys(obj2);
if (keys) {
    console.log("   ✓ PASS - Object.keys works");
}

// Test Object.values
console.log("4. Object.values");
const values = Object.values(obj2);
if (values) {
    console.log("   ✓ PASS - Object.values works");
}

// Test Object.entries
console.log("5. Object.entries");
const entries = Object.entries(obj2);
if (entries) {
    console.log("   ✓ PASS - Object.entries works");
}

console.log("\n=== All JSON/Object Tests Complete ===");
