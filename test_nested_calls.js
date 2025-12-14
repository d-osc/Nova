// Test nested function calls
console.log("=== Nested Function Call Tests ===");

// Test 1: Simple nested call
function inner() {
    console.log("Inner called");
    return 42;
}

function outer(value) {
    console.log("Outer called with:", value);
    return value * 2;
}

console.log("\nTest 1: Simple nested call");
const result = outer(inner());
console.log("Result:", result);

console.log("\n=== Test Complete ===");
