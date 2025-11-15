// Test bitwise and shift compound assignment operators
function main(): number {
    // Test &= (AND assignment)
    let a = 15;      // 1111 in binary
    a &= 7;          // 0111 in binary, result: 0111 = 7

    // Test |= (OR assignment)
    let b = 8;       // 1000 in binary
    b |= 4;          // 0100 in binary, result: 1100 = 12

    // Test ^= (XOR assignment)
    let c = 10;      // 1010 in binary
    c ^= 6;          // 0110 in binary, result: 1100 = 12

    // Test <<= (Left shift assignment)
    let d = 3;
    d <<= 2;         // 3 * 4 = 12

    // Test >>= (Right shift assignment)
    let e = 32;
    e >>= 2;         // 32 / 4 = 8

    // Test >>>= (Unsigned right shift assignment)
    let f = 64;
    f >>>= 3;        // 64 / 8 = 8

    // Result: 7 + 12 + 12 + 12 + 8 + 8 = 59
    return a + b + c + d + e + f;
}
