// Test array.length after push/pop operations
function main(): number {
    let arr = [10, 20, 30];
    let len1 = arr.length;  // Should be 3

    arr.push(40);
    let len2 = arr.length;  // Should be 4

    arr.pop();
    let len3 = arr.length;  // Should be 3

    // Return: len1 + len2 + len3 = 3 + 4 + 3 = 10
    return len1 + len2 + len3;
}
