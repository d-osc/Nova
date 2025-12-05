// Nova EventEmitter Benchmark - Final Optimized Version
import { EventEmitter } from 'nova:events';

console.log('=== Nova EventEmitter Benchmark (Optimized) ===');
console.log('');

// Global counters to avoid closure issues
let globalCounter1 = 0;
let globalCounter2 = 0;
let globalCounter3 = 0;

// Named functions to avoid arrow function issues
function listener1() {
    globalCounter1 = globalCounter1 + 1;
}

function listener2() {
    globalCounter2 = globalCounter2 + 1;
}

function listener3() {
    globalCounter3 = globalCounter3 + 1;
}

// Test 1: Add Listeners Performance
console.log('[Test 1] Add Listeners Performance');
const emitter1 = new EventEmitter();
const start1 = Date.now();

for (let i = 0; i < 1000; i++) {
    emitter1.on('test', listener1);
}

const end1 = Date.now();
const duration1 = end1 - start1;
console.log('Added 1000 listeners');
console.log('Duration: ' + duration1);

// Test 2: Emit Performance
console.log('');
console.log('[Test 2] Emit Performance (1 listener)');
const emitter2 = new EventEmitter();
emitter2.on('data', listener2);

const start2 = Date.now();

for (let i = 0; i < 10000; i++) {
    emitter2.emit('data');
}

const end2 = Date.now();
const duration2 = end2 - start2;
console.log('Emitted 10000 events');
console.log('Duration: ' + duration2);
console.log('Listener calls: ' + globalCounter2);

// Test 3: Multiple Listeners
console.log('');
console.log('[Test 3] Emit to 3 Listeners');
const emitter3 = new EventEmitter();
emitter3.on('event', listener1);
emitter3.on('event', listener2);
emitter3.on('event', listener3);

globalCounter1 = 0;
globalCounter2 = 0;
globalCounter3 = 0;

const start3 = Date.now();

for (let i = 0; i < 5000; i++) {
    emitter3.emit('event');
}

const end3 = Date.now();
const duration3 = end3 - start3;
console.log('Emitted 5000 events to 3 listeners');
console.log('Duration: ' + duration3);
console.log('Total listener calls: ' + (globalCounter1 + globalCounter2 + globalCounter3));

// Test 4: listenerCount Performance
console.log('');
console.log('[Test 4] listenerCount Performance');
const emitter4 = new EventEmitter();

for (let i = 0; i < 10; i++) {
    emitter4.on('test', listener1);
}

const start4 = Date.now();

for (let i = 0; i < 10000; i++) {
    const count = emitter4.listenerCount('test');
}

const end4 = Date.now();
const duration4 = end4 - start4;
console.log('Called listenerCount 10000 times');
console.log('Duration: ' + duration4);

// Test 5: Once Listeners
console.log('');
console.log('[Test 5] Once Listeners');
const emitter5 = new EventEmitter();

globalCounter1 = 0;

const start5 = Date.now();

for (let i = 0; i < 1000; i++) {
    emitter5.once('once-event', listener1);
    emitter5.emit('once-event');
}

const end5 = Date.now();
const duration5 = end5 - start5;
console.log('Added and triggered 1000 once-listeners');
console.log('Duration: ' + duration5);
console.log('Once calls: ' + globalCounter1);

console.log('');
console.log('=== Benchmark Complete ===');
console.log('');
console.log('Summary:');
console.log('- Test 1 (Add): ' + duration1 + 'ms');
console.log('- Test 2 (Emit): ' + duration2 + 'ms');
console.log('- Test 3 (Multi): ' + duration3 + 'ms');
console.log('- Test 4 (Count): ' + duration4 + 'ms');
console.log('- Test 5 (Once): ' + duration5 + 'ms');
