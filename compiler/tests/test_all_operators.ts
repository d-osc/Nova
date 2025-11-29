// Test ALL JavaScript Operators

function main(): number {
    let errors = 0;

    // === ARITHMETIC ===
    if (10 + 5 != 15) errors = errors + 1;     // Addition
    if (10 - 5 != 5) errors = errors + 1;      // Subtraction
    if (10 * 5 != 50) errors = errors + 1;     // Multiplication
    if (10 / 2 != 5) errors = errors + 1;      // Division
    if (10 % 3 != 1) errors = errors + 1;      // Remainder
    if (2 ** 3 != 8) errors = errors + 1;      // Exponentiation

    // === UNARY ===
    if (-5 != -5) errors = errors + 1;         // Unary negation
    if (+5 != 5) errors = errors + 1;          // Unary plus
    let inc = 5;
    inc++;
    if (inc != 6) errors = errors + 1;         // Postfix increment
    ++inc;
    if (inc != 7) errors = errors + 1;         // Prefix increment
    inc--;
    if (inc != 6) errors = errors + 1;         // Postfix decrement
    --inc;
    if (inc != 5) errors = errors + 1;         // Prefix decrement
    if (!false != true) errors = errors + 1;   // Logical NOT
    if (~0 != -1) errors = errors + 1;         // Bitwise NOT

    // === COMPARISON ===
    if ((5 == 5) != true) errors = errors + 1;   // Equal
    if ((5 != 3) != true) errors = errors + 1;   // Not equal
    if ((5 === 5) != true) errors = errors + 1;  // Strict equal
    if ((5 !== 3) != true) errors = errors + 1;  // Strict not equal
    if ((5 > 3) != true) errors = errors + 1;    // Greater than
    if ((3 < 5) != true) errors = errors + 1;    // Less than
    if ((5 >= 5) != true) errors = errors + 1;   // Greater or equal
    if ((5 <= 5) != true) errors = errors + 1;   // Less or equal

    // === LOGICAL ===
    if ((true && true) != true) errors = errors + 1;   // AND
    if ((false || true) != true) errors = errors + 1;  // OR
    // Nullish coalescing (a ?? b) - requires null support

    // === BITWISE ===
    if ((5 & 3) != 1) errors = errors + 1;     // AND
    if ((5 | 3) != 7) errors = errors + 1;     // OR
    if ((5 ^ 3) != 6) errors = errors + 1;     // XOR
    if ((2 << 2) != 8) errors = errors + 1;    // Left shift
    if ((8 >> 2) != 2) errors = errors + 1;    // Right shift
    if ((8 >>> 2) != 2) errors = errors + 1;   // Unsigned right shift

    // === ASSIGNMENT ===
    let a = 10;
    a += 5; if (a != 15) errors = errors + 1;  // Add assign
    a -= 5; if (a != 10) errors = errors + 1;  // Sub assign
    a *= 2; if (a != 20) errors = errors + 1;  // Mul assign
    a /= 4; if (a != 5) errors = errors + 1;   // Div assign
    a %= 3; if (a != 2) errors = errors + 1;   // Mod assign
    a **= 3; if (a != 8) errors = errors + 1;  // Exp assign
    a &= 3; if (a != 0) errors = errors + 1;   // AND assign
    a = 5;
    a |= 2; if (a != 7) errors = errors + 1;   // OR assign
    a ^= 3; if (a != 4) errors = errors + 1;   // XOR assign
    a <<= 1; if (a != 8) errors = errors + 1;  // Left shift assign
    a >>= 1; if (a != 4) errors = errors + 1;  // Right shift assign
    a >>>= 1; if (a != 2) errors = errors + 1; // Unsigned right shift assign

    // === TERNARY ===
    let t = 5 > 3 ? 10 : 20;
    if (t != 10) errors = errors + 1;

    // === COMMA ===
    let c = (1, 2, 3);
    if (c != 3) errors = errors + 1;

    // === GROUPING ===
    if ((2 + 3) * 4 != 20) errors = errors + 1;

    return errors;
}
