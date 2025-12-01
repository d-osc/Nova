// Test just shift
function main(): number {
    let arr = [10, 20, 30];
    let first = arr.shift();  // first = 10, arr = [20, 30]
    // Return: first + arr[0] + arr[1]
    return first + arr[0] + arr[1];  // Should be 10 + 20 + 30 = 60
}
