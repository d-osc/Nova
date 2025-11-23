// Test Number() constructor function
function main(): number {
    // Number() converts values to numbers
    // For integer type system, it's essentially a pass-through
    let a = Number(42);      // 42
    let b = Number(0);       // 0
    let c = Number(-25);     // -25
    let d = Number(100);     // 100

    // Result: 42 + 0 + (-25) + 100 = 117
    return a + b + c + d;
}
