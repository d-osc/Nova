function main(): number {
    let arr = [1, 2, 3, 4, 5];
    let hasLarge = arr.some((x) => x > 3);
    // Should return true (1) because 4 and 5 are > 3
    return hasLarge;
}
