// Test unary minus operator (-)
function main(): number {
    // Unary minus on positive number
    let a = -5;          // -5

    // Double negation
    let b = -(-10);      // 10

    // Unary minus on expression result
    let c = -(3 + 4);    // -7

    // Combined with positive
    let d = -2 + 15;     // 13

    // Result: -5 + 10 + (-7) + 13 = 11
    return a + b + c + d;
}
