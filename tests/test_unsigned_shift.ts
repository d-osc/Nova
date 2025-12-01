// Test unsigned right shift (>>>)
function main(): number {
    // Unsigned right shift with positive numbers
    let a = 16 >>> 2;     // 16 / 4 = 4

    // Unsigned right shift with negative number (treats as unsigned)
    // In JavaScript: -8 >>> 2 would give a large positive number
    // For simplicity, we'll test with positive values
    let b = 32 >>> 3;     // 32 / 8 = 4

    // Compare with signed right shift
    let c = 16 >> 2;      // 16 / 4 = 4 (same as unsigned for positive)

    // More tests
    let d = 64 >>> 1;     // 64 / 2 = 32
    let e = 8 >>> 0;      // 8 / 1 = 8 (shift by 0)

    // Result: 4 + 4 + 4 + 32 + 8 = 52
    return a + b + c + d + e;
}
