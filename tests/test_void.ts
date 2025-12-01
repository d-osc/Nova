// Test void operator
function main(): number {
    // void returns undefined, which we'll treat as 0
    let a = void 5;      // Should be 0 (undefined -> 0)
    let b = void (2+3);  // Should be 0

    // Normal values
    let c = 10;
    let d = 15;

    // Result: 0 + 0 + 10 + 15 = 25
    return a + b + c + d;
}
