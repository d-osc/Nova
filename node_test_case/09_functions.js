// Test 09: Functions
// Tests function declarations, expressions, arrow functions

console.log('=== Test 09: Functions ===');

// Function declaration
function add(a, b) {
    return a + b;
}
console.log('add(5, 3):', add(5, 3));

// Function expression
const multiply = function(a, b) {
    return a * b;
};
console.log('multiply(4, 5):', multiply(4, 5));

// Arrow functions
const subtract = (a, b) => a - b;
console.log('subtract(10, 3):', subtract(10, 3));

const square = x => x * x;
console.log('square(7):', square(7));

const greet = name => {
    return 'Hello, ' + name;
};
console.log('greet("Nova"):', greet('Nova'));

// Default parameters
function power(base, exponent = 2) {
    let result = 1;
    for (let i = 0; i < exponent; i++) {
        result *= base;
    }
    return result;
}
console.log('power(3):', power(3));
console.log('power(3, 3):', power(3, 3));

// Rest parameters
function sum(...numbers) {
    let total = 0;
    for (const num of numbers) {
        total += num;
    }
    return total;
}
console.log('sum(1, 2, 3):', sum(1, 2, 3));
console.log('sum(1, 2, 3, 4, 5):', sum(1, 2, 3, 4, 5));

// Higher-order functions
function applyOperation(a, b, operation) {
    return operation(a, b);
}
console.log('applyOperation(5, 3, add):', applyOperation(5, 3, add));
console.log('applyOperation(5, 3, multiply):', applyOperation(5, 3, multiply));

// Closures
function makeCounter() {
    let count = 0;
    return function() {
        count++;
        return count;
    };
}
const counter = makeCounter();
console.log('counter():', counter());
console.log('counter():', counter());
console.log('counter():', counter());

// Recursive function
function factorial(n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
console.log('factorial(5):', factorial(5));

// IIFE (Immediately Invoked Function Expression)
const result = (function() {
    return 42;
})();
console.log('IIFE result:', result);

console.log('\n✓ Functions Test Complete');
