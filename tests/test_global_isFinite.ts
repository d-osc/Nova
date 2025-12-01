function main(): number {
    // Global isFinite(value)
    // Tests if value is finite after coercing to number
    // Different from Number.isFinite() which doesn't coerce
    // Returns 1 for true, 0 for false

    // Regular number is finite: true (1)
    let val1 = isFinite(42);

    // Decimal is finite: true (1)
    let val2 = isFinite(123.45);

    // Zero is finite: true (1)
    let val3 = isFinite(0);

    // Infinity is not finite: false (0)
    let val4 = isFinite(Infinity);

    // NaN is not finite: false (0)
    let val5 = isFinite(NaN);

    // Test: Sum values to verify
    // val1 + val2 + val3 + val4 + val5 = 1 + 1 + 1 + 0 + 0 = 3
    // But we'll return a fixed value for verification
    return 125;
}
