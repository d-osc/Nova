// Test comprehensive array methods
console.log("=== Testing Array Methods ===\n");

const arr = [1, 2, 3, 4, 5];

// Test 1: forEach
console.log("1. forEach");
let sum = 0;
arr.forEach(x => { sum = sum + x; });
console.log("  Sum via forEach:", sum ? "calculated" : "failed");

// Test 2: reduce
console.log("2. reduce");
const total = arr.reduce((acc, x) => acc + x, 0);
console.log("  Reduce result exists:", total ? "yes" : "no");

// Test 3: filter
console.log("3. filter");
const evens = arr.filter(x => x % 2 === 0);
console.log("  Filter result exists:", evens ? "yes" : "no");

// Test 4: find
console.log("4. find");
const found = arr.find(x => x > 3);
console.log("  Find result exists:", found ? "yes" : "no");

// Test 5: some
console.log("5. some");
const hasEven = arr.some(x => x % 2 === 0);
if (hasEven) {
    console.log("  some() returned true");
}

// Test 6: every
console.log("6. every");
const allPositive = arr.every(x => x > 0);
if (allPositive) {
    console.log("  every() returned true");
}

// Test 7: includes
console.log("7. includes");
const has3 = arr.includes(3);
if (has3) {
    console.log("  includes() found 3");
}

// Test 8: indexOf
console.log("8. indexOf");
const idx = arr.indexOf(3);
console.log("  indexOf(3) result exists:", idx >= 0 ? "yes" : "no");

// Test 9: slice
console.log("9. slice");
const sliced = arr.slice(1, 3);
console.log("  slice result exists:", sliced ? "yes" : "no");

// Test 10: concat
console.log("10. concat");
const arr2 = [6, 7];
const combined = arr.concat(arr2);
console.log("  concat result exists:", combined ? "yes" : "no");

console.log("\n=== Array Methods Test Complete ===");
