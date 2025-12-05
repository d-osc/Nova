// Simple Compute Benchmark - Nova compatible

console.log('=== Simple Compute Benchmark ===');
console.log('');

// Test 1: Loop Performance
console.log('[Test 1] Loop Performance');
const start1 = Date.now();
let total1 = 0;
for (let i = 0; i < 10000000; i++) {
    total1 += i;
}
const end1 = Date.now();
console.log('Total: ' + total1);
console.log('Duration: ' + (end1 - start1) + 'ms');
console.log('');

// Test 2: Arithmetic
console.log('[Test 2] Arithmetic Operations');
const start2 = Date.now();
let result = 0;
for (let i = 0; i < 1000000; i++) {
    result = i * 2 + i * 3 - i / 2;
}
const end2 = Date.now();
console.log('Result: ' + result);
console.log('Duration: ' + (end2 - start2) + 'ms');
console.log('');

// Test 3: Fibonacci (Iterative)
console.log('[Test 3] Fibonacci (Iterative)');
function fibIter(n) {
    if (n <= 1) return n;
    let a = 0;
    let b = 1;
    for (let i = 2; i <= n; i++) {
        const temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

const start3 = Date.now();
const fib40 = fibIter(40);
const end3 = Date.now();
console.log('Fib(40): ' + fib40);
console.log('Duration: ' + (end3 - start3) + 'ms');
console.log('');

console.log('=== Benchmark Complete ===');
