// Nova EventEmitter Benchmark - Without Arrow Functions
import { EventEmitter } from 'nova:events';

console.log('=== Nova EventEmitter Benchmark (No Arrow Functions) ===');

// Test 1: Basic functionality
console.log('[Test 1] Basic EventEmitter Test');
const emitter1 = new EventEmitter();

console.log('Created EventEmitter');
console.log('Emitter created successfully!');

console.log('=== Benchmark Complete ===');
