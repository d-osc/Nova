function main(): number {
    let arr = [1, 2, 3, 4, 5];
    let sum = arr.reduce((acc, x) => acc + x, 0);
    // sum should be 0 + 1 + 2 + 3 + 4 + 5 = 15
    return sum;
}
