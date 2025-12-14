// Minimal closure test
function makeCounter() {
    let count = 0;
    return function() {
        count++;
        return count;
    };
}

const counter = makeCounter();
console.log("First call:", counter());   // Should be 1
console.log("Second call:", counter());  // Should be 2
console.log("Third call:", counter());   // Should be 3
