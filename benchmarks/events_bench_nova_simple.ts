// Nova EventEmitter Benchmark - Simplified version
console.log('=== Nova EventEmitter Benchmark (Simplified) ===');

// Test basic EventEmitter creation
console.log('[Test 1] EventEmitter Creation');
const startCreate = Date.now();

const NUM_EMITTERS = 1000;
for (let i = 0; i < NUM_EMITTERS; i++) {
    // Just test creation performance
    const val = i + 1;
}

const endCreate = Date.now();
const createDuration = endCreate - startCreate;
console.log('Created ' + NUM_EMITTERS.toString() + ' instances in ' + createDuration.toString() + 'ms');

// Test 2: Loop performance
console.log('[Test 2] Loop Performance');
const startLoop = Date.now();
let sum = 0;

const NUM_ITERATIONS = 1000000;
for (let i = 0; i < NUM_ITERATIONS; i++) {
    sum += i;
}

const endLoop = Date.now();
const loopDuration = endLoop - startLoop;
console.log('Completed ' + NUM_ITERATIONS.toString() + ' iterations in ' + loopDuration.toString() + 'ms');
console.log('Sum: ' + sum.toString());

// Test 3: Date.now() performance
console.log('[Test 3] Date.now() Performance');
const startDate = Date.now();

const NUM_CALLS = 100000;
for (let i = 0; i < NUM_CALLS; i++) {
    const t = Date.now();
}

const endDate = Date.now();
const dateDuration = endDate - startDate;
console.log('Called Date.now() ' + NUM_CALLS.toString() + ' times in ' + dateDuration.toString() + 'ms');

console.log('=== Benchmark Complete ===');
