// Nova EventEmitter Benchmark - Working Version (No Arrow Functions)
import { EventEmitter } from 'nova:events';

console.log('=== Nova EventEmitter Benchmark ===');
console.log('');

// Test 1: Add/Remove Listeners
console.log('[Test 1] Add/Remove Listeners Performance');
const emitter1 = new EventEmitter();
const startAdd = Date.now();

const NUM_LISTENERS = 1000;  // Reduced from 10000 for simpler test
let addCounter = 0;

function listener1() {
    addCounter++;
}

for (let i = 0; i < NUM_LISTENERS; i++) {
    emitter1.on('test', listener1);
}

const midAdd = Date.now();
const addDuration = midAdd - startAdd;
console.log('Added ' + NUM_LISTENERS.toString() + ' listeners in ' + addDuration.toString() + 'ms');
if (addDuration > 0) {
    const throughput = NUM_LISTENERS / (addDuration / 1000);
    console.log('Throughput: ' + throughput.toFixed(0) + ' listeners/sec');
}

// Test 2: Emit Performance
console.log('');
console.log('[Test 2] Emit Performance (1 listener)');
const emitter2 = new EventEmitter();
let emitCounter = 0;

function listener2() {
    emitCounter++;
}

emitter2.on('data', listener2);

const NUM_EMITS = 10000;
const startEmit = Date.now();

for (let i = 0; i < NUM_EMITS; i++) {
    emitter2.emit('data');
}

const endEmit = Date.now();
const emitDuration = endEmit - startEmit;
console.log('Emitted ' + NUM_EMITS.toString() + ' events in ' + emitDuration.toString() + 'ms');
if (emitDuration > 0) {
    const throughput = NUM_EMITS / (emitDuration / 1000);
    console.log('Throughput: ' + throughput.toFixed(0) + ' emits/sec');
}
console.log('Total listener calls: ' + emitCounter.toString());

// Test 3: Multiple Listeners
console.log('');
console.log('[Test 3] Multiple Listeners');
const emitter3 = new EventEmitter();
let multiCounter = 0;

function listener3a() { multiCounter++; }
function listener3b() { multiCounter++; }
function listener3c() { multiCounter++; }

emitter3.on('event', listener3a);
emitter3.on('event', listener3b);
emitter3.on('event', listener3c);

const NUM_MULTI = 5000;
const startMulti = Date.now();

for (let i = 0; i < NUM_MULTI; i++) {
    emitter3.emit('event');
}

const endMulti = Date.now();
const multiDuration = endMulti - startMulti;
console.log('Emitted ' + NUM_MULTI.toString() + ' events to 3 listeners in ' + multiDuration.toString() + 'ms');
if (multiDuration > 0) {
    const throughput = NUM_MULTI / (multiDuration / 1000);
    console.log('Throughput: ' + throughput.toFixed(0) + ' emits/sec');
}
console.log('Total listener calls: ' + multiCounter.toString());

// Test 4: listenerCount
console.log('');
console.log('[Test 4] listenerCount Performance');
const emitter4 = new EventEmitter();

function listener4() {}

for (let i = 0; i < 10; i++) {
    emitter4.on('test', listener4);
}

const NUM_COUNTS = 10000;
const startCount = Date.now();

for (let i = 0; i < NUM_COUNTS; i++) {
    const count = emitter4.listenerCount('test');
}

const endCount = Date.now();
const countDuration = endCount - startCount;
console.log('Called listenerCount ' + NUM_COUNTS.toString() + ' times in ' + countDuration.toString() + 'ms');
if (countDuration > 0) {
    const throughput = NUM_COUNTS / (countDuration / 1000);
    console.log('Throughput: ' + throughput.toFixed(0) + ' ops/sec');
}

console.log('');
console.log('=== Benchmark Complete ===');
