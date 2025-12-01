function main(): number {
    let arr = [10, 20, 30, 40, 50];
    // Test positive index: arr.at(2) = 30
    let a = arr.at(2);
    // Test negative index: arr.at(-1) = 50 (last element)
    let b = arr.at(-1);
    // Test negative index: arr.at(-2) = 40 (second to last)
    let c = arr.at(-2);
    // Return sum: 30 + 50 + 40 = 120
    return a + b + c;
}
