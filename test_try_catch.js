// Try-catch test
let result = 0;

try {
    result = 42;
    console.log("in try, result:", result);
} catch (e) {
    console.log("in catch");
    result = 99;
}

console.log("final result:", result);
console.log("\nExpected:");
console.log("in try, result: 42");
console.log("final result: 42");
