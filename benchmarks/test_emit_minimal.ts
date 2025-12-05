// Minimal test for emit functionality
import { EventEmitter } from 'nova:events';

console.log('Test 1: Create emitter');
const emitter = new EventEmitter();
console.log('OK - Emitter created');

console.log('Test 2: Define listener');
function myListener() {
    console.log('Listener called');
}
console.log('OK - Listener defined');

console.log('Test 3: Add listener');
emitter.on('test', myListener);
console.log('OK - Listener added');

console.log('Test 4: Emit event');
emitter.emit('test');
console.log('OK - Event emitted');

console.log('All tests passed!');
