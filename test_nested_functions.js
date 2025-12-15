// Nested functions
function outer(x) {
    function inner(y) {
        return x + y;
    }
    return inner(10);
}

const result = outer(5);
console.log("result:", result);
console.log("Expected: result: 15");
