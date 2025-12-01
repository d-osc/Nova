function main(): number {
    let arr = [10, 20, 30];
    let reversed = arr.toReversed();
    // reversed = [30, 20, 10]
    // arr = [10, 20, 30] (unchanged)

    // Test: reversed[0] + arr[0] = 30 + 10 = 40
    return reversed[0] + arr[0];
}
