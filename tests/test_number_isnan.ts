// Test Number.isNaN() function
function main(): number {
    // Number.isNaN() checks if a value is NaN (Not a Number)
    // For i64 integer type system, there is no NaN, so always returns 0 (false)
    let a = Number.isNaN(42);       // 0 (false)
    let b = Number.isNaN(-100);     // 0 (false)
    let c = Number.isNaN(0);        // 0 (false)
    let d = Number.isNaN(999);      // 0 (false)

    // Result: 0 + 0 + 0 + 0 = 0
    return a + b + c + d;
}
