function makeCounter() {
    let count = 0;
    return function() {
        count = count + 1;
        return count;
    };
}

const counter = makeCounter();
const result1 = counter();
const result2 = counter();

console.log("First call: " + result1);
console.log("Second call: " + result2);
