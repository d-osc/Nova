// Test various JavaScript features to see what's supported

console.log("=== Testing JavaScript Features ===\n");

// 1. Arrow functions
console.log("Test 1: Arrow functions");
try {
    const add = (a, b) => a + b;
    console.log("  ✓ Arrow functions work");
} catch (e) {
    console.log("  ✗ Arrow functions don't work");
}

// 2. Array methods
console.log("\nTest 2: Array methods");
const arr = [1, 2, 3];
console.log("  Array created:", arr);

// 3. Template literals
console.log("\nTest 3: Template literals");
const name = "Nova";
console.log(`  Hello ${name}`);

// 4. Destructuring
console.log("\nTest 4: Destructuring");
const [a, b] = [1, 2];
console.log("  a =", a, "b =", b);

// 5. Spread operator
console.log("\nTest 5: Spread operator");
const arr2 = [...arr, 4, 5];
console.log("  Spread result:", arr2);

// 6. for...of loop
console.log("\nTest 6: for...of loop");
for (const item of arr) {
    console.log("  Item:", item);
}

// 7. typeof operator
console.log("\nTest 7: typeof");
console.log("  typeof 5:", typeof 5);
console.log("  typeof 'hello':", typeof "hello");

// 8. JSON methods
console.log("\nTest 8: JSON");
const obj = { x: 1, y: 2 };
const json = JSON.stringify(obj);
console.log("  JSON.stringify:", json);
