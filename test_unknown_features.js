// Test features marked as "Unknown" to see if they work

console.log("=== Testing Unknown Features ===");

// 1. For-of loop
console.log("\n1. For-of loop:");
const arr = [10, 20, 30];
for (const val of arr) {
    console.log("Value:", val);
}

// 2. Do-while loop
console.log("\n2. Do-while loop:");
let i = 0;
do {
    console.log("i =", i);
    i = i + 1;
} while (i < 3);

console.log("\n=== Tests complete ===");
