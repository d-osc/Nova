console.log("Test: Callback debugging");

// Simple test - does callback get called at all?
const numbers = [10, 20, 30];

// Test with explicit return
const found = numbers.find((x) => {
    console.log("  Callback called with:", x);
    return x > 15;
});

console.log("Found:", found);
console.log("Done");
