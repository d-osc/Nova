// Comprehensive Expressions and Operators Test

function main(): number {
    let errors = 0;

    // 1. Arithmetic operators
    let a = 10 + 5;      // Addition
    let b = 10 - 5;      // Subtraction
    let c = 10 * 5;      // Multiplication
    let d = 10 / 2;      // Division
    let e = 10 % 3;      // Modulo
    let f = 2 ** 3;      // Exponentiation

    if (a != 15) errors = errors + 1;
    if (b != 5) errors = errors + 1;
    if (c != 50) errors = errors + 1;
    if (d != 5) errors = errors + 1;
    if (e != 1) errors = errors + 1;
    if (f != 8) errors = errors + 1;

    // 2. Comparison operators
    let cmp1 = 5 == 5;   // Equal
    let cmp2 = 5 != 3;   // Not equal
    let cmp3 = 5 > 3;    // Greater than
    let cmp4 = 5 < 10;   // Less than
    let cmp5 = 5 >= 5;   // Greater or equal
    let cmp6 = 5 <= 5;   // Less or equal

    // 3. Logical operators
    let log1 = true && true;   // AND
    let log2 = false || true;  // OR
    let log3 = !false;         // NOT

    // 4. Bitwise operators
    let bit1 = 5 & 3;    // AND (101 & 011 = 001)
    let bit2 = 5 | 3;    // OR (101 | 011 = 111)
    let bit3 = 5 ^ 3;    // XOR (101 ^ 011 = 110)
    let bit4 = ~0;       // NOT
    let bit5 = 2 << 2;   // Left shift (2 << 2 = 8)
    let bit6 = 8 >> 2;   // Right shift (8 >> 2 = 2)
    let bit7 = 8 >>> 2;  // Unsigned right shift

    if (bit1 != 1) errors = errors + 1;
    if (bit2 != 7) errors = errors + 1;
    if (bit3 != 6) errors = errors + 1;
    if (bit5 != 8) errors = errors + 1;
    if (bit6 != 2) errors = errors + 1;
    if (bit7 != 2) errors = errors + 1;

    // 5. Unary operators
    let un1 = -5;        // Unary minus
    let un2 = +5;        // Unary plus

    if (un1 != -5) errors = errors + 1;
    if (un2 != 5) errors = errors + 1;

    // 6. Increment/Decrement
    let inc = 5;
    inc++;               // Postfix increment
    if (inc != 6) errors = errors + 1;

    ++inc;               // Prefix increment
    if (inc != 7) errors = errors + 1;

    inc--;               // Postfix decrement
    if (inc != 6) errors = errors + 1;

    --inc;               // Prefix decrement
    if (inc != 5) errors = errors + 1;

    // 7. Assignment operators
    let assign = 10;
    assign += 5;         // Add assign
    if (assign != 15) errors = errors + 1;

    assign -= 5;         // Sub assign
    if (assign != 10) errors = errors + 1;

    assign *= 2;         // Mul assign
    if (assign != 20) errors = errors + 1;

    assign /= 4;         // Div assign
    if (assign != 5) errors = errors + 1;

    assign %= 3;         // Mod assign
    if (assign != 2) errors = errors + 1;

    // 8. Ternary operator
    let tern = 5 > 3 ? 1 : 0;
    if (tern != 1) errors = errors + 1;

    // 9. typeof operator
    let typ = typeof 42;

    // 10. Comma operator
    let comma = (1, 2, 3);
    if (comma != 3) errors = errors + 1;

    return errors;
}
