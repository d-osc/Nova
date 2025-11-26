// Test Array.prototype.fill() method
function main(): number {
    // Array.fill() fills array elements with a static value
    let arr = [1, 2, 3, 4, 5];
    arr.fill(7);  // [7, 7, 7, 7, 7]

    // Result: arr[0] + arr[2] + arr[4]
    //       = 7 + 7 + 7 = 21
    return arr[0] + arr[2] + arr[4];
}
