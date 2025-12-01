// Test unary plus operator (+)
function main(): number {
    // Unary plus on positive number (no-op for numbers)
    let a = +5;          // 5

    // Unary plus on negative number
    let b = +(-10);      // -10

    // Unary plus on expression result
    let c = +(3 + 4);    // 7

    // Unary plus with variable
    let x = 8;
    let d = +x;          // 8

    // Combined operations
    let e = +2 + +3;     // 5

    // Result: 5 + (-10) + 7 + 8 + 5 = 15
    return a + b + c + d + e;
}
