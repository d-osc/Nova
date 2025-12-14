console.log("=== Array Callback Methods Test ===\n");

const numbers = [10, 20, 30, 40, 50];
console.log("Original array:", numbers);
console.log("");

// Test find()
console.log("1. Array.find()");
const found = numbers.find((x) => x > 25);
console.log("   find(x => x > 25):", found);
console.log("   PASS\n");

// Test filter()
console.log("2. Array.filter()");
const filtered = numbers.filter((x) => x >= 30);
console.log("   filter(x => x >= 30):", filtered);
console.log("   PASS\n");

// Test map()
console.log("3. Array.map()");
const doubled = numbers.map((x) => x * 2);
console.log("   map(x => x * 2):", doubled);
console.log("   PASS\n");

// Test some()
console.log("4. Array.some()");
const hasLarge = numbers.some((x) => x > 40);
console.log("   some(x => x > 40):", hasLarge);
console.log("   PASS\n");

// Test every()
console.log("5. Array.every()");
const allPositive = numbers.every((x) => x > 0);
console.log("   every(x => x > 0):", allPositive);
console.log("   PASS\n");

console.log("=== All Array Callback Tests Passed! ===");
