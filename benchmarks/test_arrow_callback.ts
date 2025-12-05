// Test arrow function as callback
console.log('Testing arrow as callback...');

let counter = 0;

const fn = () => {
    counter++;
};

fn();
console.log('Counter: ' + counter.toString());
