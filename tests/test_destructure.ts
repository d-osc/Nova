// Test destructuring
function main(): number {
    // Array destructuring
    let arr = [1, 2, 3];
    let [a, b, c] = arr;
    if (a != 1) return 1;
    if (b != 2) return 2;
    return 0;
}
