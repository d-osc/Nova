// Test closures/nested functions

console.log("=== Test 1: Simple nested function ===");
function outer(x) {
    function inner(y) {
        return x + y;
    }
    return inner(10);
}
const result1 = outer(5);
console.log("Result:", result1, "Expected: 15");

console.log("=== Test 2: Nested function with multiple variables ===");
function makeAdder(a) {
    function add(b) {
        return a + b;
    }
    return add;
}
const add5 = makeAdder(5);
const result2 = add5(3);
console.log("Result:", result2, "Expected: 8");

console.log("=== Test 3: Deep nesting ===");
function level1(x) {
    function level2(y) {
        function level3(z) {
            return x + y + z;
        }
        return level3(3);
    }
    return level2(2);
}
const result3 = level1(1);
console.log("Result:", result3, "Expected: 6");
