// Math tests - Nova test format
// Uses return code: 0 = pass, non-zero = fail

function main(): number {
    let failures = 0;

    // Basic arithmetic
    if (2 + 3 !== 5) failures = failures + 1;
    if (10 - 4 !== 6) failures = failures + 1;
    if (7 * 8 !== 56) failures = failures + 1;
    if (20 / 4 !== 5) failures = failures + 1;

    // Math.sqrt
    if (Math.sqrt(16) !== 4) failures = failures + 1;
    if (Math.sqrt(25) !== 5) failures = failures + 1;
    if (Math.sqrt(100) !== 10) failures = failures + 1;

    // Math.abs
    if (Math.abs(-10) !== 10) failures = failures + 1;
    if (Math.abs(5) !== 5) failures = failures + 1;
    if (Math.abs(0) !== 0) failures = failures + 1;

    return failures;
}
