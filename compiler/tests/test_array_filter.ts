function main(): number {
    let arr = [1, 2, 3, 4, 5];
    let filtered = arr.filter((x) => x > 3);
    // filtered should be [4, 5]
    // Return length to verify (should be 2)
    return filtered.length;
}
