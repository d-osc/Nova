function main(): number {
    // Math.log2() returns base 2 logarithm
    // For integer type system, returns integer approximation
    // log2(1) = 0 (always)
    // log2(2) = 1
    // log2(4) = 2
    // log2(8) = 3
    // log2(16) = 4
    let a = Math.log2(2);      // 1
    let b = Math.log2(8);      // 3
    let c = Math.log2(16);     // 4
    // Result: 1 + 3 + 4 = 8
    return a + b + c;
}
