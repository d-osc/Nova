// Simple performance test for Nova
console.log('=== Nova Performance Test ===');

// Test 1: Loop performance
const START = Date.now();
let sum = 0;
for (let i = 0; i < 1000000; i++) {
    sum += i;
}
const END = Date.now();
const duration = END - START;

console.log('Loop test completed');
console.log('Sum: ' + sum.toString());
console.log('Duration: ' + duration.toString() + ' ms');

console.log('=== Test Complete ===');
