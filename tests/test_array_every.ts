function main(): number {
    let arr = [1, 2, 3, 4, 5];
    let allSmall = arr.every((x) => x < 10);
    // Should return true (1) because all elements are < 10
    return allSmall;
}
