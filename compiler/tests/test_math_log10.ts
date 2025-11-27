function main(): number {
    // Math.log10() returns base 10 logarithm
    // For integer type system, returns integer approximation
    // log10(1) = 0 (always)
    // log10(10) = 1
    // log10(100) = 2
    // log10(1000) = 3
    let a = Math.log10(10);     // 1
    let b = Math.log10(100);    // 2
    let c = Math.log10(1000);   // 3
    // Result: 1 + 2 + 3 = 6
    return a + b + c;
}
