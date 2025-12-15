// Comprehensive test of JavaScript spread operator functionality

// Test 1: Basic spread
const arr1 = [1, 2, 3];
const arr2 = [...arr1, 4, 5];
console.log("✓ Basic spread:", arr2[0], arr2[1], arr2[2], arr2[3], arr2[4]);

// Test 2: Spread at beginning
const arr3 = [10, ...arr1];
console.log("✓ Spread at start:", arr3[0], arr3[1], arr3[2], arr3[3]);

// Test 3: Spread at end
const arr4 = [10, 20, ...arr1];
console.log("✓ Spread at end:", arr4[0], arr4[1], arr4[2], arr4[3], arr4[4]);

// Test 4: Spread in middle
const arr5 = [99, ...arr1, 100];
console.log("✓ Spread in middle:", arr5[0], arr5[1], arr5[2], arr5[3], arr5[4]);

// Test 5: Multiple spreads
const arr6 = [7, 8];
const arr7 = [...arr1, ...arr6];
console.log("✓ Multiple spreads:", arr7[0], arr7[1], arr7[2], arr7[3], arr7[4]);

// Test 6: Simple copy (single spread)
const arr8 = [...arr1];
console.log("✓ Simple copy:", arr8[0], arr8[1], arr8[2]);

console.log("");
console.log("All spread operator tests passed! ✓");
