function main(): number {
    let arr = [10, 20, 30];
    let reversed = arr.toReversed();

    let a = reversed[0];
    let b = arr[0];

    return a + b;  // Should be 30 + 10 = 40
}
