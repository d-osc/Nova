// Nova EventEmitter Benchmark - Simple Version
import { EventEmitter } from 'nova:events';

console.log('=== Nova EventEmitter Benchmark ===');
console.log('');

// Test 1: Object Creation Performance
console.log('[Test 1] EventEmitter Creation Performance');
const startCreate = Date.now();

const NUM_EMITTERS = 1000;
for (let i = 0; i < NUM_EMITTERS; i++) {
    const e = new EventEmitter();
}

const endCreate = Date.now();
const createDuration = endCreate - startCreate;
console.log('Created ' + NUM_EMITTERS.toString() + ' EventEmitters in ' + createDuration.toString() + 'ms');
if (createDuration > 0) {
    const throughput = NUM_EMITTERS / (createDuration / 1000);
    console.log('Throughput: ' + throughput.toFixed(0) + ' objects/sec');
}

// Test 2: Method Resolution Performance
console.log('');
console.log('[Test 2] Method Resolution Performance');
const emitter = new EventEmitter();
const startResolve = Date.now();

const NUM_RESOLVES = 100000;
for (let i = 0; i < NUM_RESOLVES; i++) {
    const m1 = emitter.on;
    const m2 = emitter.emit;
    const m3 = emitter.listenerCount;
}

const endResolve = Date.now();
const resolveDuration = endResolve - startResolve;
console.log('Resolved methods ' + NUM_RESOLVES.toString() + ' times in ' + resolveDuration.toString() + 'ms');
if (resolveDuration > 0) {
    const throughput = (NUM_RESOLVES * 3) / (resolveDuration / 1000);
    console.log('Throughput: ' + throughput.toFixed(0) + ' resolutions/sec');
}

// Test 3: Basic Operations
console.log('');
console.log('[Test 3] Basic Loop Performance');
const startLoop = Date.now();
let sum = 0;

const NUM_ITERATIONS = 1000000;
for (let i = 0; i < NUM_ITERATIONS; i++) {
    sum += i;
}

const endLoop = Date.now();
const loopDuration = endLoop - startLoop;
console.log('Completed ' + NUM_ITERATIONS.toString() + ' iterations in ' + loopDuration.toString() + 'ms');
if (loopDuration > 0) {
    const throughput = NUM_ITERATIONS / (loopDuration / 1000);
    console.log('Throughput: ' + throughput.toFixed(0) + ' ops/sec');
}
console.log('Sum: ' + sum.toString());

console.log('');
console.log('=== Benchmark Complete ===');
console.log('');
console.log('Note: Full EventEmitter benchmarks require callback support');
console.log('Current status: Object creation and method resolution working!');
