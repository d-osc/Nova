// Simple nested call test - no printing parameters
console.log("=== Simple Nested Call Test ===");

function inner() {
    return 42;
}

function outer(value) {
    return value * 2;
}

const result = outer(inner());
console.log("Result:", result);

console.log("=== Test Complete ===");
