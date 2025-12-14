const arr1 = [1, 2, 3];

// Test 1: Spread at the end
const test1 = [10, 20, ...arr1];
console.log("Test 1 (spread at end):", test1[0], test1[1], test1[2], test1[3], test1[4]);

// Test 2: Spread in the middle  
const test2 = [99, ...arr1, 100];
console.log("Test 2 (spread in middle):", test2[0], test2[1], test2[2], test2[3], test2[4]);

// Test 3: Multiple spreads
const arr2 = [7, 8];
const test3 = [...arr1, ...arr2];
console.log("Test 3 (multiple spreads):", test3[0], test3[1], test3[2], test3[3], test3[4]);
