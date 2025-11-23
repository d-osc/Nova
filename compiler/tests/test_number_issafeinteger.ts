// Test Number.isSafeInteger() function
function main(): number {
    // Number.isSafeInteger() checks if a value is a safe integer
    // For i64 integer type system, all values are safe integers
    let a = Number.isSafeInteger(42);       // 1 (true)
    let b = Number.isSafeInteger(-100);     // 1 (true)
    let c = Number.isSafeInteger(0);        // 1 (true)
    let d = Number.isSafeInteger(999);      // 1 (true)
    let e = Number.isSafeInteger(-50);      // 1 (true)

    // Result: 1 + 1 + 1 + 1 + 1 = 5
    return a + b + c + d + e;
}
