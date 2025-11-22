// Test array.reverse() and access all elements
function main(): number {
    let arr = [1, 2, 3, 4, 5];
    arr.reverse();

    // After reverse: [5, 4, 3, 2, 1]
    // Sum: 5 + 4 + 3 + 2 + 1 = 15
    return arr[0] + arr[1] + arr[2] + arr[3] + arr[4];
}
