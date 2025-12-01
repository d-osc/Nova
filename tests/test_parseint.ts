// Test parseInt() global function
function main(): number {
    // For now, with integer type system, parseInt just returns the value
    let a = parseInt(42);
    let b = parseInt(100);
    let c = parseInt(-50);

    // Result: 42 + 100 + (-50) = 92
    return a + b + c;
}
