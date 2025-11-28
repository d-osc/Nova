// Test Number constants - v1.3.59
function main(): number {
    // Test all Number constants can be accessed
    let maxVal = Number.MAX_VALUE;
    let minVal = Number.MIN_VALUE;
    let eps = Number.EPSILON;
    let posInf = Number.POSITIVE_INFINITY;
    let negInf = Number.NEGATIVE_INFINITY;
    let nanVal = Number.NaN;

    // Also test the existing constants still work
    let maxSafe = Number.MAX_SAFE_INTEGER;
    let minSafe = Number.MIN_SAFE_INTEGER;

    // Return success code
    return 209;
}
