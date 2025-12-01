function main(): number {
    // Array.prototype.toReversed() - ES2023
    // Returns a NEW reversed array
    // Original array is NOT modified (immutable operation)

    let arr = [10, 20, 30, 40, 50];
    //         0   1   2   3   4

    // Create reversed copy
    let reversed = arr.toReversed();
    // reversed = [50, 40, 30, 20, 10]
    //             0   1   2   3   4

    // Test: reversed[0] + reversed[4]
    //     = 50 + 10 = 60
    return reversed[0] + reversed[4];
}
