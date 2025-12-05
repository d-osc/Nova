// Universal Compute Benchmark - Works on Node, Bun, Deno, Nova

console.log('=== Compute Benchmark ===');
console.log('');

// Test 1: Fibonacci (Recursive)
console.log('[Test 1] Fibonacci (Recursive)');
function fib(n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

const start1 = Date.now();
const result1 = fib(35);
const end1 = Date.now();
console.log('Result: ' + result1);
console.log('Duration: ' + (end1 - start1) + 'ms');
console.log('');

// Test 2: Array Operations
console.log('[Test 2] Array Operations');
const start2 = Date.now();
let arr = [];
for (let i = 0; i < 100000; i++) {
    arr.push(i);
}
const sum = arr.reduce((a, b) => a + b, 0);
const end2 = Date.now();
console.log('Sum: ' + sum);
console.log('Duration: ' + (end2 - start2) + 'ms');
console.log('');

// Test 3: String Operations
console.log('[Test 3] String Operations');
const start3 = Date.now();
let str = '';
for (let i = 0; i < 10000; i++) {
    str += 'x';
}
const end3 = Date.now();
console.log('Length: ' + str.length);
console.log('Duration: ' + (end3 - start3) + 'ms');
console.log('');

// Test 4: Object Creation
console.log('[Test 4] Object Creation');
const start4 = Date.now();
let objects = [];
for (let i = 0; i < 100000; i++) {
    objects.push({ x: i, y: i * 2, z: i * 3 });
}
const end4 = Date.now();
console.log('Objects: ' + objects.length);
console.log('Duration: ' + (end4 - start4) + 'ms');
console.log('');

// Test 5: Loop Performance
console.log('[Test 5] Loop Performance');
const start5 = Date.now();
let total = 0;
for (let i = 0; i < 10000000; i++) {
    total += i;
}
const end5 = Date.now();
console.log('Total: ' + total);
console.log('Duration: ' + (end5 - start5) + 'ms');
console.log('');

console.log('=== Benchmark Complete ===');
