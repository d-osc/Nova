console.log("Testing default with number:");

function add(a, b = 10) {
    console.log("a:", a, "b:", b);
    return a + b;
}

console.log("Result 1:", add(5, 3));
console.log("Result 2:", add(7));
console.log("Done");
