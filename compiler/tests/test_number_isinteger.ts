// Test Number.isInteger() function
function main(): number {
    // Number.isInteger() checks if a value is an integer
    // For i64 integer type system, all values are integers, so always returns 1 (true)
    let a = Number.isInteger(42);       // 1 (true)
    let b = Number.isInteger(-100);     // 1 (true)
    let c = Number.isInteger(0);        // 1 (true)
    let d = Number.isInteger(999);      // 1 (true)
    let e = Number.isInteger(-50);      // 1 (true)

    // Result: 1 + 1 + 1 + 1 + 1 = 5
    return a + b + c + d + e;
}
