// Test if the property resolution fix works
import { EventEmitter } from 'nova:events';

console.log('Testing EventEmitter with property resolution fix...');

const emitter = new EventEmitter();

console.log('Created EventEmitter');
console.log('Testing method resolution...');

// These should now resolve correctly without warnings
const result1 = emitter.on;
const result2 = emitter.emit;
const result3 = emitter.listenerCount;

console.log('Method resolution test complete!');
