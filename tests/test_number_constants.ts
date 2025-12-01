function main(): number {
    // Number constants (ES1/ES2015)

    // Test Number.MAX_SAFE_INTEGER (2^53 - 1)
    let maxSafe = Number.MAX_SAFE_INTEGER;
    console.log(maxSafe);  // Should print: 9007199254740991

    // Test Number.MIN_SAFE_INTEGER (-(2^53 - 1))
    let minSafe = Number.MIN_SAFE_INTEGER;
    console.log(minSafe);  // Should print: -9007199254740991

    // Return success code
    return 202;
}
