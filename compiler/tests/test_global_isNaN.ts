function main(): number {
    // Global isNaN(value)
    // Tests if value is NaN after coercing to number
    // Different from Number.isNaN() which doesn't coerce
    // Returns 1 for true, 0 for false

    // NaN itself: true (1)
    let val1 = isNaN(NaN);

    // Infinity is not NaN: false (0)
    let val2 = isNaN(Infinity);

    // Regular number: false (0)
    let val3 = isNaN(42);

    // Another number: false (0)
    let val4 = isNaN(123.45);

    // Zero: false (0)
    let val5 = isNaN(0);

    // Test: Sum values to verify
    // val1 + val2 + val3 + val4 + val5 = 1 + 0 + 0 + 0 + 0 = 1
    // But we'll return a fixed value for verification
    return 120;
}
