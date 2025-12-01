// Test just push and pop
function main(): number {
    let arr = [10, 20, 30];

    arr.push(40);  // arr = [10, 20, 30, 40]
    let last = arr.pop();  // last = 40, arr = [10, 20, 30]

    // Return: arr[0] + arr[1] + arr[2] + last
    // Should be: 10 + 20 + 30 + 40 = 100
    return arr[0] + arr[1] + arr[2] + last;
}
