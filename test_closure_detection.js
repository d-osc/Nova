function makeCounter() {
    let count = 0;
    return function() {
        count = count + 1;
        return count;
    };
}

const counter = makeCounter();
console.log(counter());
console.log(counter());
