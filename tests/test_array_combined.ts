// Combined test: array element access + array.length
function main(): number {
    let arr = [10, 20, 30];

    // Test element access
    let sum1 = arr[0] + arr[1] + arr[2];  // Should be 60

    // Test length before modification
    let len1 = arr.length;  // Should be 3

    // Modify array
    arr.push(40);

    // Test element access after modification
    let elem = arr[3];  // Should be 40

    // Test length after modification
    let len2 = arr.length;  // Should be 4

    // Return: 60 + 3 + 40 + 4 = 107
    return sum1 + len1 + elem + len2;
}
