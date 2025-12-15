// Simple array method tests
console.log("Testing array methods...\n");

const arr = [1, 2, 3, 4, 5];

// Test includes (no callback)
console.log("1. includes(3)");
const has3 = arr.includes(3);
if (has3) {
    console.log("   ✓ PASS");
} else {
    console.log("   ✗ FAIL");
}

// Test indexOf (no callback)
console.log("2. indexOf(3)");
const idx = arr.indexOf(3);
if (idx >= 0) {
    console.log("   ✓ PASS - found at index");
} else {
    console.log("   ✗ FAIL");
}

// Test slice (no callback)
console.log("3. slice(1, 3)");
const sliced = arr.slice(1, 3);
if (sliced) {
    console.log("   ✓ PASS - sliced array created");
} else {
    console.log("   ✗ FAIL");
}

// Test concat (no callback)
console.log("4. concat([6, 7])");
const arr2 = [6, 7];
const combined = arr.concat(arr2);
if (combined) {
    console.log("   ✓ PASS - concatenated");
} else {
    console.log("   ✗ FAIL");
}

// Test map (with callback)
console.log("5. map(x => x * 2)");
const doubled = arr.map(x => x * 2);
if (doubled) {
    console.log("   ✓ PASS - mapped array created");
} else {
    console.log("   ✗ FAIL");
}

console.log("\nDone!");
