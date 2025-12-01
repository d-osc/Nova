// Test inline reverse
function main(): number {
    let arr = [1, 2, 3];
    arr.reverse();
    return arr[0] + arr[2];  // Should be 3 + 1 = 4
}
