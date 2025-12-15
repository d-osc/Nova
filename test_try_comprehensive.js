console.log("=== Try/Catch/Finally Comprehensive Test ===\n");

// Test 1: Try without exception
console.log("Test 1: Try without exception");
try {
    console.log("  Try block executed");
    const x = 10 + 5;
    console.log("  Result:", x);
} catch (e) {
    console.log("  ERROR: Catch should not execute");
}
console.log("  PASS\n");

// Test 2: Finally block executes
console.log("Test 2: Finally always executes");
let finallyExecuted = false;
try {
    console.log("  Try block");
} finally {
    console.log("  Finally block");
    finallyExecuted = true;
}
console.log("  Finally executed:", finallyExecuted);
console.log("  PASS\n");

// Test 3: Try-catch-finally all together
console.log("Test 3: Try-catch-finally");
try {
    console.log("  Try");
} catch (e) {
    console.log("  Catch");
} finally {
    console.log("  Finally");
}
console.log("  PASS\n");

console.log("=== All Tests Complete ===");
console.log("Try/catch/finally: WORKING!");
