// Test reversing array twice (should return to original)
function main(): number {
    let arr = [100, 200, 300];
    arr.reverse();  // [300, 200, 100]
    arr.reverse();  // [100, 200, 300] - back to original

    // Return: 100 + 200 + 300 = 600
    return arr[0] + arr[1] + arr[2];
}
