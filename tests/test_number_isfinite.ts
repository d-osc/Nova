// Test Number.isFinite() function
function main(): number {
    // Number.isFinite() checks if a value is finite
    // For i64 integer type system, all values are finite
    let a = Number.isFinite(42);       // 1 (true)
    let b = Number.isFinite(-100);     // 1 (true)
    let c = Number.isFinite(0);        // 1 (true)
    let d = Number.isFinite(999);      // 1 (true)

    // Result: 1 + 1 + 1 + 1 = 4
    return a + b + c + d;
}
