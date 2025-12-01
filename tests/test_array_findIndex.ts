function main(): number {
    let arr = [10, 20, 30, 40, 50];
    // Find index of first element > 25
    let idx = arr.findIndex((x) => x > 25);
    // Should return 2 (index of 30)
    return idx;
}
