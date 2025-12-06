// Test: Function declarations + Object literals in loops
console.log("Testing function + object combination...");

function multiply(a, b) {
    return a * b;
}

var results = [];
for (var i = 0; i < 10; i++) {
    results.push({
        index: i,
        value: multiply(i, 2),
        squared: i * i
    });
}

console.log("Created " + results.length + " objects");
console.log("First: " + results[0].value);
console.log("Last: " + results[9].value);
console.log("All tests passed!");
