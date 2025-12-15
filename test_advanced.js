// Test more advanced features

// 1. Closures
function makeCounter() {
    let count = 0;
    return function() {
        count = count + 1;
        return count;
    };
}
const counter = makeCounter();
console.log("Counter:", counter());
console.log("Counter:", counter());

// 2. Rest parameters
function sum(...numbers) {
    let total = 0;
    for (let i = 0; i < numbers.length; i = i + 1) {
        total = total + numbers[i];
    }
    return total;
}
console.log("Sum:", sum(1, 2, 3, 4));
