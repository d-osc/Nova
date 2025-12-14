console.log("=== Quick Feature Tests ===\n");

// Test 1: typeof operator  
console.log("Test 1: typeof");
const x = 42;
const typeResult = typeof x;
console.log("  typeof 42:", typeResult);

// Test 2: Logical operators
console.log("\nTest 2: Logical operators");
const a = true && false;
const b = true || false;  
const c = !true;
console.log("  true && false:", a);
console.log("  true || false:", b);
console.log("  !true:", c);

// Test 3: Comparison operators
console.log("\nTest 3: Comparisons");
const eq = (5 === 5);
const neq = (5 !== 6);
const gt = (10 > 5);
console.log("  5 === 5:", eq);
console.log("  5 !== 6:", neq);
console.log("  10 > 5:", gt);

// Test 4: Math operations
console.log("\nTest 4: Math operations");
const sum = 10 + 5;
const product = 10 * 5;
const modulo = 10 % 3;
console.log("  10 + 5:", sum);
console.log("  10 * 5:", product);
console.log("  10 % 3:", modulo);

console.log("\n=== Done ===");
