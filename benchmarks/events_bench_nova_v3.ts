// Nova EventEmitter Benchmark - Simple Version
import { EventEmitter } from 'nova:events';

console.log('=== Nova EventEmitter Benchmark ===');

// Test 1: Object Creation Performance
console.log('[Test 1] EventEmitter Creation');
const startCreate = Date.now();

const NUM_EMITTERS = 1000;
for (let i = 0; i < NUM_EMITTERS; i++) {
    const e = new EventEmitter();
}

const endCreate = Date.now();
const createDuration = endCreate - startCreate;
console.log('Created 1000 EventEmitters');
console.log('Duration: ' + createDuration + 'ms');

// Test 2: Method Access
console.log('[Test 2] Method Access');
const emitter = new EventEmitter();

const m1 = emitter.on;
const m2 = emitter.emit;
const m3 = emitter.listenerCount;

console.log('Method access successful');

// Test 3: Basic Performance
console.log('[Test 3] Loop Performance');
const startLoop = Date.now();
let sum = 0;

for (let i = 0; i < 1000000; i++) {
    sum += i;
}

const endLoop = Date.now();
const loopDuration = endLoop - startLoop;
console.log('Completed 1000000 iterations');
console.log('Duration: ' + loopDuration + 'ms');

console.log('=== Benchmark Complete ===');
