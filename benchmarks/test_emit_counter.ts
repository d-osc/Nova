// Test listener with counter
import { EventEmitter } from 'nova:events';

console.log('Test: Listener with counter');

let counter = 0;

function incrementCounter() {
    counter = counter + 1;
}

const emitter = new EventEmitter();
emitter.on('count', incrementCounter);

console.log('Counter before: ' + counter);

emitter.emit('count');
emitter.emit('count');
emitter.emit('count');

console.log('Counter after: ' + counter);

if (counter == 3) {
    console.log('SUCCESS - Counter working!');
} else {
    console.log('FAILED - Counter not working');
}
