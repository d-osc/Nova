// Test String() constructor function
function main(): number {
    // String() converts values to strings
    // For now, with integer type system, we'll just return the value
    // (string operations will be added later with proper string support)
    let a = String(42);      // Would be "42" as string, but returns 42 for now
    let b = String(0);       // Would be "0" as string, but returns 0 for now
    let c = String(75);      // Would be "75" as string, but returns 75 for now

    // Result: 42 + 0 + 75 = 117
    return a + b + c;
}
