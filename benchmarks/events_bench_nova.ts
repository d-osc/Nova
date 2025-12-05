// Nova EventEmitter Benchmark
import { EventEmitter } from 'nova:events';

console.log('=== Nova EventEmitter Benchmark ===');

// Test 1: Add/Remove Listeners
console.log('[Test 1] Add/Remove Listeners Performance');
const emitter1 = new EventEmitter();
const startAdd = Date.now();

const NUM_LISTENERS = 10000;
const listeners = [];
for (let i = 0; i < NUM_LISTENERS; i++) {
    const fn = () => {};
    listeners.push(fn);
    emitter1.on('test', fn);
}

const midAdd = Date.now();
const addDuration = midAdd - startAdd;
console.log('Added ' + NUM_LISTENERS.toString() + ' listeners in ' + addDuration.toString() + 'ms');
console.log('Throughput: ' + (NUM_LISTENERS / (addDuration / 1000)).toFixed(0) + ' listeners/sec');

// Remove listeners
for (let i = 0; i < NUM_LISTENERS; i++) {
    emitter1.off('test', listeners[i]);
}

const endAdd = Date.now();
const removeDuration = endAdd - midAdd;
console.log('Removed ' + NUM_LISTENERS.toString() + ' listeners in ' + removeDuration.toString() + 'ms');
console.log('Throughput: ' + (NUM_LISTENERS / (removeDuration / 1000)).toFixed(0) + ' listeners/sec');

// Test 2: Emit Performance (10 listeners)
console.log('[Test 2] Emit Performance (10 listeners)');
const emitter2 = new EventEmitter();
let callCount = 0;

for (let i = 0; i < 10; i++) {
    emitter2.on('data', () => { callCount++; });
}

const NUM_EMITS = 100000;
const startEmit = Date.now();

for (let i = 0; i < NUM_EMITS; i++) {
    emitter2.emit('data');
}

const endEmit = Date.now();
const emitDuration = endEmit - startEmit;
console.log('Emitted ' + NUM_EMITS.toString() + ' events in ' + emitDuration.toString() + 'ms');
console.log('Throughput: ' + (NUM_EMITS / (emitDuration / 1000)).toFixed(0) + ' emits/sec');
console.log('Total listener calls: ' + callCount.toString());

// Test 3: Once Listeners
console.log('[Test 3] Once Listeners Performance');
const emitter3 = new EventEmitter();
let onceCallCount = 0;

const startOnce = Date.now();
const NUM_ONCE = 10000;

for (let i = 0; i < NUM_ONCE; i++) {
    emitter3.once('event', () => { onceCallCount++; });
}

// Emit once to trigger all
emitter3.emit('event');

const endOnce = Date.now();
const onceDuration = endOnce - startOnce;
console.log('Added and triggered ' + NUM_ONCE.toString() + ' once-listeners in ' + onceDuration.toString() + 'ms');
console.log('Throughput: ' + (NUM_ONCE / (onceDuration / 1000)).toFixed(0) + ' ops/sec');
console.log('Once calls: ' + onceCallCount.toString());

// Test 4: Multiple Events
console.log('[Test 4] Multiple Event Types');
const emitter4 = new EventEmitter();
const NUM_EVENTS = 100;
const NUM_LISTENERS_PER_EVENT = 10;

const startMulti = Date.now();

// Add listeners for multiple events
for (let i = 0; i < NUM_EVENTS; i++) {
    const eventName = 'event' + i.toString();
    for (let j = 0; j < NUM_LISTENERS_PER_EVENT; j++) {
        emitter4.on(eventName, () => {});
    }
}

const midMulti = Date.now();
const multiAddDuration = midMulti - startMulti;
console.log('Added ' + (NUM_EVENTS * NUM_LISTENERS_PER_EVENT).toString() + ' listeners across ' + NUM_EVENTS.toString() + ' events in ' + multiAddDuration.toString() + 'ms');

// Emit all events
for (let i = 0; i < NUM_EVENTS; i++) {
    emitter4.emit('event' + i.toString());
}

const endMulti = Date.now();
const multiEmitDuration = endMulti - midMulti;
console.log('Emitted ' + NUM_EVENTS.toString() + ' different events in ' + multiEmitDuration.toString() + 'ms');
console.log('Total operations: ' + (NUM_EVENTS * NUM_LISTENERS_PER_EVENT + NUM_EVENTS).toString());

// Test 5: Emit with Arguments
console.log('[Test 5] Emit with Arguments');
const emitter5 = new EventEmitter();
let argSum = 0;

for (let i = 0; i < 10; i++) {
    emitter5.on('calculate', (a, b, c) => {
        argSum += a + b + c;
    });
}

const NUM_ARG_EMITS = 50000;
const startArgs = Date.now();

for (let i = 0; i < NUM_ARG_EMITS; i++) {
    emitter5.emit('calculate', 1, 2, 3);
}

const endArgs = Date.now();
const argsDuration = endArgs - startArgs;
console.log('Emitted ' + NUM_ARG_EMITS.toString() + ' events with 3 arguments in ' + argsDuration.toString() + 'ms');
console.log('Throughput: ' + (NUM_ARG_EMITS / (argsDuration / 1000)).toFixed(0) + ' emits/sec');
console.log('Arg sum: ' + argSum.toString());

// Test 6: listenerCount
console.log('[Test 6] listenerCount Performance');
const emitter6 = new EventEmitter();

for (let i = 0; i < 100; i++) {
    emitter6.on('test', () => {});
}

const NUM_COUNTS = 100000;
const startCount = Date.now();

for (let i = 0; i < NUM_COUNTS; i++) {
    emitter6.listenerCount('test');
}

const endCount = Date.now();
const countDuration = endCount - startCount;
console.log('Called listenerCount ' + NUM_COUNTS.toString() + ' times in ' + countDuration.toString() + 'ms');
console.log('Throughput: ' + (NUM_COUNTS / (countDuration / 1000)).toFixed(0) + ' ops/sec');

console.log('=== Benchmark Complete ===');
